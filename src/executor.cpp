#include "executor.h"
#include "board.h"
#include "common.h"
#include "creation.h"
#include "eval.h"
#include "eval_antichess.h"
#include "eval_standard.h"
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

} // namespace

bool Executor::MatchResult(vector<string>* response) {
  int result = player_builder_->evaluator->Result();
  if (!(result == WIN || result == -WIN || result == DRAW)) {
    return false;
  }
  string match_result;
  if (result == WIN) {
    if (player_builder_->board->SideToMove() == Side::WHITE) {
      match_result = "1-0 {White Wins}";
    } else {
      match_result = "0-1 {Black Wins}";
    }
  } else if (result == -WIN) {
    if (player_builder_->board->SideToMove() == Side::WHITE) {
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

void Executor::ReBuildPlayer() {
  switch (variant_) {
  case Variant::STANDARD:
    player_builder_.reset(new StandardPlayerBuilder());
    break;
  case Variant::ANTICHESS:
    player_builder_.reset(new AntichessPlayerBuilder(pns_));
    break;
  }
  assert(player_builder_.get() != nullptr);
  PlayerBuilderDirector director(player_builder_.get());
  BuildOptions options;
  options.init_fen = init_fen_;
  player_ = director.Build(options);
  assert(player_ != nullptr);
}

void Executor::ReBuildPonderer() {
  switch (variant_) {
  case Variant::STANDARD:
    ponderer_builder_.reset(new StandardPlayerBuilder());
    break;
  case Variant::ANTICHESS:
    ponderer_builder_.reset(new AntichessPlayerBuilder(false));
    break;
  }
  assert(ponderer_builder_.get() != nullptr);
  if (!player_) {
    ReBuildPlayer();
  }
  PlayerBuilderDirector director(ponderer_builder_.get());
  BuildOptions options;
  options.init_fen = player_builder_->board->ParseIntoFEN();
  options.transpos = player_builder_->transpos.get();
  ponderer_ = director.Build(options);
  assert(ponderer_ != nullptr);
}

void Executor::StartPondering(double time_centis) {
  if (!ponder_) {
    return;
  }
  // Don't ponder if too little time remaining or if already pondering.
  if (time_centis < 500 || pondering_thread_) {
    return;
  }
  ReBuildPonderer();
  pondering_thread_.reset(new std::thread([this] {
    SearchParams ponder_params;
    ponder_params.thinking_output = false;
    // Just seeding transposition table for now.
    this->ponderer_->Search(ponder_params, 300 * 100);
  }));
}

void Executor::StopPondering() {
  // If already not pondering, then no-op.
  if (!pondering_thread_) {
    return;
  }
  ponderer_builder_->timer->Invalidate();
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
    pns_ = true;
    think_time_centis_ = -1;
    search_params_.search_depth = MAX_DEPTH;
    ReBuildPlayer();
  } else if (cmd == "variant") {
    StopPondering();
    force_mode_ = false;
    variant_ = Variant::STANDARD;
    if (cmd_parts.at(1) == "suicide" || cmd_parts.at(1) == "giveaway" ||
        cmd_parts.at(1) == "S") {
      variant_ = Variant::ANTICHESS;
    }
    ReBuildPlayer();
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
    ReBuildPlayer();
  } else if (cmd == "go") {
    if (player_ == nullptr) {
      ReBuildPlayer();
    }
    StopPondering();
    if (!MatchResult(response)) {
      force_mode_ = false;
      Move cmove = player_->Search(search_params_, AllocateTime());
      player_builder_->board->MakeMove(cmove);
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
    if (player_ == nullptr) {
      ReBuildPlayer();
    }
    StopPondering();
    Move move(cmd_parts.at(1));
    if (force_mode_) {
      std::cout << "# Forced: " << player_builder_->board->ParseIntoFEN() << "|"
                << move.str() << std::endl;
      player_builder_->board->MakeMove(move);
      OutputFEN();
    } else if (!player_builder_->movegen->IsValidMove(move)) {
      response->push_back("Illegal move: " + move.str());
    } else {
      player_builder_->board->MakeMove(move);
      OutputFEN();
      if (!MatchResult(response)) {
        Move cmove = player_->Search(search_params_, AllocateTime());
        player_builder_->board->MakeMove(cmove);
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
    player_builder_->board->DebugPrintBoard();
  } else if (cmd == "unmake") {
    player_builder_->board->UnmakeLastMove();
  } else if (cmd == "movelist") {
    MoveGenerator* movegen = nullptr;
    switch (variant_) {
    case Variant::STANDARD:
      movegen = new MoveGeneratorStandard(player_builder_->board.get());
      break;
    case Variant::ANTICHESS:
      movegen = new MoveGeneratorAntichess(*player_builder_->board);
      break;
    }
    MoveArray move_array;
    movegen->GenerateMoves(&move_array);
    for (size_t i = 0; i < move_array.size(); ++i) {
      std::cout << "# " << i + 1 << " " << move_array.get(i).str() << std::endl;
    }
    delete movegen;
  } else if (cmd == "nopns") {
    pns_ = false;
    ReBuildPlayer();
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

  if (quit_ && player_builder_.get()) {
    StopPondering();
    player_builder_->transpos->LogStats();
    if (player_builder_->egtb) {
      player_builder_->egtb->LogStats();
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
  std::cout << "# FEN: " << player_builder_->board->ParseIntoFEN() << std::endl;
}
