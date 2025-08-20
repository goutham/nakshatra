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
  } else if (cmd == "level") {
    // XBoard level command: "level MOVES BASE INC"
    // MOVES = moves per time control (0 = all moves)
    // BASE = base time in minutes:seconds or minutes
    // INC = increment per move in seconds
    if (cmd_parts.size() >= 4) {
      try {
        movestogo_ = StringToInt(cmd_parts.at(1));
        
        // Parse base time (can be "minutes" or "minutes:seconds")
        const std::string& base_str = cmd_parts.at(2);
        size_t colon_pos = base_str.find(':');
        double base_minutes = 0;
        if (colon_pos != std::string::npos) {
          // Format: "minutes:seconds"
          int minutes = StringToInt(base_str.substr(0, colon_pos));
          int seconds = StringToInt(base_str.substr(colon_pos + 1));
          base_minutes = minutes + seconds / 60.0;
        } else {
          // Format: "minutes"
          base_minutes = StringToInt(base_str);
        }
        
        // Convert to centiseconds and set both sides' time
        double base_centis = base_minutes * 60 * 100;
        time_centis_ = base_centis;
        otime_centis_ = base_centis;
        
        // Parse increment in seconds, convert to centiseconds
        double inc_seconds = std::stod(cmd_parts.at(3));
        inc_centis_ = static_cast<int>(inc_seconds * 100);
        
        std::cout << "# Level set: " << movestogo_ << " moves, " 
                  << base_minutes << " minutes, " << inc_seconds << " sec increment" << std::endl;
      } catch (const std::exception& e) {
        std::cout << "# Error parsing level command" << std::endl;
      }
    }
  } else if (cmd == "Error" || cmd == "feature" ||
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
  
  // In XBoard protocol: time = engine's time, otim = opponent's time
  double our_time_centis = time_centis_;
  double our_inc_centis = inc_centis_;
  
  long calculated_time_centis = 0;
  if (our_time_centis > 0) {
    if (movestogo_ > 0) {
      // If moves to go is specified, divide remaining time by moves plus buffer
      calculated_time_centis = static_cast<long>(our_time_centis / (movestogo_ + 2) + our_inc_centis);
    } else {
      // Default: use 1/30 of remaining time plus increment
      calculated_time_centis = static_cast<long>(our_time_centis / 30 + our_inc_centis);
    }
    
    // Minimum 10 centis (0.1s), maximum 1/3 of remaining time for safety
    calculated_time_centis = std::max(10l, std::min(calculated_time_centis, static_cast<long>(our_time_centis / 3)));
  } else {
    // Fallback if no valid time information
    calculated_time_centis = 100l; // 1 second default
  }
  
  std::cout << "# Time allocation: " << calculated_time_centis << " centis" 
            << " (time=" << our_time_centis << ", inc=" << our_inc_centis 
            << ", movestogo=" << movestogo_ << ")" << std::endl;
  return calculated_time_centis;
}

void Executor::OutputFEN() const {
  std::cout << "# FEN: " << main_context_->board->ParseIntoFEN() << std::endl;
}
