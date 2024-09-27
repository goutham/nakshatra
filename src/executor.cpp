#include "executor.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "player.h"
#include "transpos.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using std::string;
using std::vector;

namespace {

int TransposSize(const Variant variant) {
  if (IsStandard(variant)) {
    return STANDARD_TRANSPOS_SIZE;
  }
  assert(IsAntichessLike(variant));
  return ANTICHESS_TRANSPOS_SIZE;
}

} // namespace

ExecutionContext::ExecutionContext(const Variant variant,
                                   const ExecutionContext::Options& options)
    : variant(variant) {
  timer = std::make_unique<Timer>();
  if (options.init_fen.empty()) {
    board = std::make_unique<Board>(variant);
  } else {
    board = std::make_unique<Board>(variant, options.init_fen);
  }
  if (options.transpos) {
    transpos = std::unique_ptr<TranspositionTable>(options.transpos);
    own_transpos = false;
  } else {
    transpos = std::make_unique<TranspositionTable>(TransposSize(variant));
    own_transpos = true;
  }
  player = std::make_unique<Player>(variant, *board, *transpos, *timer);
}

ExecutionContext::~ExecutionContext() {
  if (!own_transpos) {
    transpos.release();
  }
}

bool Executor::MatchResult(vector<string>& response) {
  int result;
  if (variant_ == Variant::STANDARD) {
    result = EvalResult<Variant::STANDARD>(*main_context_->board);
  } else if (variant_ == Variant::ANTICHESS) {
    result = EvalResult<Variant::ANTICHESS>(*main_context_->board);
  } else if (variant_ == Variant::SUICIDE) {
    result = EvalResult<Variant::SUICIDE>(*main_context_->board);
  } else {
    assert(false); // unreachable
  }
  if (!(result == WIN || result == -WIN || result == DRAW)) {
    return false;
  }
  string match_result;
  if (result == WIN) {
    if (main_context_->board->SideToMove() == Side::WHITE) {
      match_result = "1-0 {White Wins}";
    } else {
      match_result = "0-1 {Black Wins}";
    }
  } else if (result == -WIN) {
    if (main_context_->board->SideToMove() == Side::WHITE) {
      match_result = "0-1 {Black Wins}";
    } else {
      match_result = "1-0 {White Wins}";
    }
  } else {
    match_result = "1/2-1/2 {Draw}";
  }
  response.push_back(match_result);
  return true;
}

void Executor::RebuildMainContext() {
  ExecutionContext::Options options;
  options.init_fen = init_fen_;
  main_context_ = std::make_unique<ExecutionContext>(variant_, options);
}

void Executor::RebuildPonderingContext() {
  if (!main_context_) {
    RebuildMainContext();
  }
  ExecutionContext::Options options;
  options.init_fen = main_context_->board->ParseIntoFEN();
  options.transpos = main_context_->transpos.get();
  pondering_context_ = std::make_unique<ExecutionContext>(variant_, options);
}

void Executor::StartPondering(double time_centis) {
  if (!ponder_) {
    return;
  }
  // Don't ponder if too little time remaining or if already pondering.
  if (time_centis < 500 || pondering_thread_) {
    return;
  }
  RebuildPonderingContext();
  pondering_thread_.reset(new std::thread([this] {
    SearchParams ponder_params;
    ponder_params.thinking_output = false;
    ponder_params.antichess_pns = false;
    // Just seeding transposition table for now.
    this->pondering_context_->player->Search(ponder_params, 300 * 100);
  }));
}

void Executor::StopPondering() {
  // If already not pondering, then no-op.
  if (!pondering_thread_) {
    return;
  }
  pondering_context_->timer->Invalidate();
  pondering_thread_->join();
  pondering_thread_.reset(nullptr);
}

vector<string> Executor::Execute(const string& command_str) {
  vector<string> response;
  if (command_str.empty()) {
    return response;
  }
  const std::vector<std::string> cmd_parts = SplitString(command_str, ' ');
  const string& cmd = cmd_parts[0];
  if (cmd == "new") {
    StopPondering();
    variant_ = Variant::STANDARD;
    force_mode_ = false;
    think_time_centis_ = -1;
    search_params_.search_depth = MAX_DEPTH;
    RebuildMainContext();
  } else if (cmd == "variant") {
    StopPondering();
    force_mode_ = false;
    variant_ = Variant::STANDARD;
    if (cmd_parts.at(1) == "giveaway") {
      variant_ = Variant::ANTICHESS;
    } else if (cmd_parts.at(1) == "suicide") {
      variant_ = Variant::SUICIDE;
    } else {
      std::cout << "# Ignoring unrecognized variant: " << cmd_parts.at(1)
                << std::endl;
    }
    RebuildMainContext();
  } else if (cmd == "sd") {
    std::stringstream ss(cmd_parts.at(1));
    ss >> search_params_.search_depth;
    std::cout << "# Search Depth = " << search_params_.search_depth
              << std::endl;
  } else if (cmd == "setboard") {
    init_fen_.clear();
    for (size_t i = 1; i < cmd_parts.size(); ++i) {
      init_fen_ += cmd_parts[i] + " ";
    }
    // Erase last character which is a trailing space.
    if (!init_fen_.empty()) {
      init_fen_.erase(init_fen_.end() - 1);
    }
    RebuildMainContext();
  } else if (cmd == "go") {
    if (!main_context_) {
      RebuildMainContext();
    }
    StopPondering();
    if (!MatchResult(response)) {
      force_mode_ = false;
      Move cmove =
          main_context_->player->Search(search_params_, AllocateTime());
      main_context_->board->MakeMove(cmove);
      OutputFEN();
      response.push_back("move " + cmove.str());
      StartPondering(time_centis_);
    }
  } else if (cmd == "thinktime") {
    think_time_centis_ = StringToInt(cmd_parts.at(1));
  } else if (cmd == "time") {
    time_centis_ = StringToInt(cmd_parts.at(1));
  } else if (cmd == "otim") {
    otime_centis_ = StringToInt(cmd_parts.at(1));
  } else if (cmd == "usermove") {
    if (!main_context_) {
      RebuildMainContext();
    }
    StopPondering();
    Move move(cmd_parts.at(1));
    if (force_mode_) {
      std::cout << "# Forced: " << main_context_->board->ParseIntoFEN() << "|"
                << move.str() << std::endl;
      main_context_->board->MakeMove(move);
      OutputFEN();
    } else if (!IsValidMove(variant_, *main_context_->board, move)) {
      response.push_back("Illegal move: " + move.str());
    } else {
      main_context_->board->MakeMove(move);
      OutputFEN();
      if (!MatchResult(response)) {
        Move cmove =
            main_context_->player->Search(search_params_, AllocateTime());
        main_context_->board->MakeMove(cmove);
        OutputFEN();
        response.push_back("move " + cmove.str());
        if (!MatchResult(response)) {
          StartPondering(time_centis_);
        }
      }
    }
  } else if (cmd == "force") {
    StopPondering();
    force_mode_ = true;
  } else if (cmd == "sb") {
    main_context_->board->DebugPrintBoard();
  } else if (cmd == "unmake") {
    main_context_->board->UnmakeLastMove();
  } else if (cmd == "easy") {
    ponder_ = false;
  } else if (cmd == "hard") {
    ponder_ = true;
  } else if (cmd == "nopost") {
    search_params_.thinking_output = false;
  } else if (cmd == "ping") {
    response.push_back("pong " + cmd_parts.at(1));
  } else if (cmd == "post") {
    search_params_.thinking_output = true;
  } else if (cmd == "quit") {
    quit_ = true;
  } else if (cmd == "Error" || cmd == "feature" || cmd == "level" ||
             cmd == "xboard" || cmd == "accepted" || cmd == "rejected" ||
             cmd == "?" || cmd == "protover" || cmd == "sigterm" ||
             cmd == "name" || cmd == "rating" || cmd == "computer" ||
             cmd == "st" || cmd == "result" || cmd == "draw") {
    // Ignore / TBD
  } else {
    response.push_back("Error (Unknown command): " + command_str);
  }

  if (quit_ && main_context_.get()) {
    StopPondering();
    main_context_->transpos->LogStats();
    auto egtb = GetEGTB(variant_);
    if (egtb) {
      egtb->LogStats();
    }
  }
  return response;
}

// Returns time (in centis) to allocate for search.
long Executor::AllocateTime() const {
  if (think_time_centis_ > 0) {
    return think_time_centis_;
  }
  if (time_centis_ < 500) {
    return 1l;
  }
  double a, b, c, d, e;
  if (IsStandard(variant_)) {
    a = -1.50140990e-08;
    b = 7.61654331e-06;
    c = 1.86255488e-04;
    d = -5.43305966e-01;
    e = 9.66625927e+01;
  } else if (IsAntichessLike(variant_)) {
    a = 1.04978830e-06;
    b = -3.17302622e-04;
    c = 3.41067191e-02;
    d = -1.57304241e+00;
    e = 4.83298816e+01;
  } else {
    assert(false); // unreachable
  }
  const int movenum = main_context_->board->HalfMoveClock();
  const double est_self_moves_remaining =
      (std::max(a * pow(movenum, 4) + b * pow(movenum, 3) +
                    c * pow(movenum, 2) + d * movenum + e,
                20.0)) /
      2;
  const long alloc_centis =
      static_cast<long>(time_centis_ / est_self_moves_remaining);
  std::cout << "# Allocating time: " << alloc_centis << " centis" << std::endl;
  return alloc_centis;
}

void Executor::OutputFEN() const {
  std::cout << "# FEN: " << main_context_->board->ParseIntoFEN() << std::endl;
}
