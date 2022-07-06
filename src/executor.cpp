#include "executor.h"
#include "board.h"
#include "common.h"
#include "config.h"
#include "eval.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "player.h"
#include "transpos.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using std::string;
using std::vector;

namespace {

// Linear interpolation - given x1, y1, x2, y2 and x3, find y3.
double Interpolate(double x1, double y1, double x2, double y2, double x3) {
  return y1 + ((y2 - y1) / (x2 - x1)) * (x3 - x1);
}

int TransposSize(const Variant variant) {
  if (variant == Variant::STANDARD) {
    return STANDARD_TRANSPOS_SIZE;
  }
  assert(variant == Variant::ANTICHESS);
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
  player = std::make_unique<Player>(variant, board.get(), transpos.get(),
                                    timer.get());
}

ExecutionContext::~ExecutionContext() {
  if (!own_transpos) {
    transpos.release();
  }
}

bool Executor::MatchResult(vector<string>* response) {
  int result = variant_ == Variant::STANDARD
                   ? EvalResult<Variant::STANDARD>(main_context_->board.get())
                   : EvalResult<Variant::ANTICHESS>(main_context_->board.get());
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
  response->push_back(match_result);
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

void Executor::Execute(const string& command_str, vector<string>* response) {
  if (command_str.empty()) {
    return;
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
    if (cmd_parts.at(1) == "suicide" || cmd_parts.at(1) == "giveaway" ||
        cmd_parts.at(1) == "S") {
      variant_ = Variant::ANTICHESS;
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
      response->push_back("move " + cmove.str());
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
    } else if (!IsValidMove(variant_, main_context_->board.get(), move)) {
      response->push_back("Illegal move: " + move.str());
    } else {
      main_context_->board->MakeMove(move);
      OutputFEN();
      if (!MatchResult(response)) {
        Move cmove =
            main_context_->player->Search(search_params_, AllocateTime());
        main_context_->board->MakeMove(cmove);
        OutputFEN();
        response->push_back("move " + cmove.str());
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
    response->push_back("pong " + cmd_parts.at(1));
  } else if (cmd == "post") {
    search_params_.thinking_output = true;
  } else if (cmd == "quit") {
    quit_ = true;
  } else if (cmd == "Error" || cmd == "feature" || cmd == "level" ||
             cmd == "xboard" || cmd == "accepted" || cmd == "rejected" ||
             cmd == "?" || cmd == "protover" || cmd == "sigterm" ||
             cmd == "name" || cmd == "rating" || cmd == "computer" ||
             cmd == "st" || cmd == "result") {
    // Ignore / TBD
  } else {
    response->push_back("Error (Unknown command): " + command_str);
  }

  if (quit_ && main_context_.get()) {
    StopPondering();
    main_context_->transpos->LogStats();
    auto egtb = GetEGTB(variant_);
    if (egtb) {
      egtb->LogStats();
    }
  }
}

// Returns time (in centis) to allocate for search.
long Executor::AllocateTime() const {
  if (think_time_centis_ > 0) {
    return think_time_centis_;
  }
  if (time_centis_ >= 60000) {
    return 2250l;
  }
  if (time_centis_ >= 6000) {
    return Interpolate(6000, 500, 60000, 2250, time_centis_);
  }
  if (time_centis_ >= 1000) {
    return Interpolate(1000, 100, 6000, 500, time_centis_);
  }
  if (time_centis_ >= 300) {
    return Interpolate(300, 10, 1000, 100, time_centis_);
  }
  return 5l;
}

void Executor::OutputFEN() const {
  std::cout << "# FEN: " << main_context_->board->ParseIntoFEN() << std::endl;
}
