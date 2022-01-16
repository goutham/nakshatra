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

// All supported Xboard communication protocol commands.
enum CmdName {
  ERROR,
  FEATURE,
  FORCE,
  GO,
  INVALID, // if command entered is invalid.
  MOVELIST,
  NEW,
  NOBOOK,
  NOPNS,
  NOPONDER,
  NOPOST,
  OMOVELIST,
  OTIME,
  PING,
  POST,
  QUIT,
  RESULT,
  RANDMOVES,
  SEARCH_DEPTH,
  SETBOARD,
  SHOWBOARD,
  THINKTIME,
  TIME,
  UNMAKE,
  USERMOVE,
  VARIANT,
};

struct Command {
  CmdName cmd_name;
  std::vector<std::string> arguments;
};

Command Interpret(const std::string& cmd) {
  static std::map<std::string, CmdName> cmd_map = {
      {"Error", ERROR},      {"feature", FEATURE},
      {"force", FORCE},      {"go", GO},
      {"move", USERMOVE},    {"mlist", MOVELIST},
      {"omlist", OMOVELIST}, {"new", NEW},
      {"nobook", NOBOOK},    {"nopns", NOPNS},
      {"nopost", NOPOST},    {"otim", OTIME},
      {"ping", PING},        {"noponder", NOPONDER},
      {"post", POST},        {"quit", QUIT},
      {"result", RESULT},    {"randmoves", RANDMOVES},
      {"sd", SEARCH_DEPTH},  {"setboard", SETBOARD},
      {"sb", SHOWBOARD},     {"thinktime", THINKTIME},
      {"time", TIME},        {"usermove", USERMOVE},
      {"variant", VARIANT},  {"unmake", UNMAKE}};
  std::vector<std::string> parts = SplitString(cmd, ' ');
  Command command;
  if (auto cmd_map_kv = cmd_map.find(parts[0]); cmd_map_kv == cmd_map.end()) {
    command.cmd_name = INVALID;
  } else {
    command.cmd_name = cmd_map_kv->second;
  }
  for (size_t i = 1; i < parts.size(); ++i) {
    command.arguments.push_back(parts[i]);
  }
  return command;
}

// Linear interpolation - given x1, y1, x2, y2 and x3, find y3.
double Interpolate(double x1, double y1, double x2, double y2, double x3) {
  return y1 + ((y2 - y1) / (x2 - x1)) * (x3 - x1);
}

} // namespace

bool Executor::MatchResult(vector<string>* response) {
  int result = player_builder_->GetEvaluator()->Result();
  if (!(result == WIN || result == -WIN || result == DRAW)) {
    return false;
  }
  string match_result;
  if (result == WIN) {
    if (player_->GetBoard()->SideToMove() == Side::WHITE) {
      match_result = "1-0 {White Wins}";
    } else {
      match_result = "0-1 {Black Wins}";
    }
  } else if (result == -WIN) {
    if (player_->GetBoard()->SideToMove() == Side::WHITE) {
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

void Executor::ReBuildPlayer(int rand_moves) {
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
  options.build_book = book_;
  options.rand_moves = rand_moves;
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
    ReBuildPlayer(rand_moves_);
  }
  PlayerBuilderDirector director(ponderer_builder_.get());
  BuildOptions options;
  options.init_fen = player_->GetBoard()->ParseIntoFEN();
  options.transpos = player_builder_->GetTranspos();
  options.build_book = false;
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
  ponderer_builder_->GetTimer()->Invalidate();
  pondering_thread_->join();
  pondering_thread_.reset(nullptr);
}

void Executor::Execute(const string& command_str, vector<string>* response) {
  switch (Command command = Interpret(command_str); command.cmd_name) {
  case NEW: {
    StopPondering();
    variant_ = Variant::STANDARD;
    force_mode_ = false;
    pns_ = true;
    book_ = true;
    think_time_centis_ = -1;
    search_params_.search_depth = MAX_DEPTH;
    ReBuildPlayer(rand_moves_);
  } break;

  case VARIANT: {
    StopPondering();
    force_mode_ = false;
    variant_ = Variant::STANDARD;
    if (command.arguments.at(0) == "suicide" ||
        command.arguments.at(0) == "giveaway" ||
        command.arguments.at(0) == "S") {
      variant_ = Variant::ANTICHESS;
    }
    ReBuildPlayer(rand_moves_);
  } break;

  case SEARCH_DEPTH: {
    std::stringstream ss(command.arguments.at(0));
    ss >> search_params_.search_depth;
    std::cout << "# Search Depth = " << search_params_.search_depth
              << std::endl;
  } break;

  case SETBOARD: {
    init_fen_.clear();
    for (const string& part : command.arguments) {
      init_fen_ += part + " ";
    }
    // Erase last character which is a trailing space.
    if (!init_fen_.empty()) {
      init_fen_.erase(init_fen_.end() - 1);
    }
    ReBuildPlayer(rand_moves_);
  } break;

  case GO: {
    if (player_ == nullptr) {
      ReBuildPlayer(rand_moves_);
    }
    StopPondering();
    if (MatchResult(response)) {
      break;
    }
    force_mode_ = false;
    Move cmove = player_->Search(search_params_, AllocateTime());
    player_->GetBoard()->MakeMove(cmove);
    OutputFEN();
    response->push_back("move " + cmove.str());
    StartPondering(time_centis_);
  } break;

  case THINKTIME:
    think_time_centis_ = StringToInt(command.arguments.at(0));
    break;

  case TIME:
    time_centis_ = StringToInt(command.arguments.at(0));
    break;

  case OTIME:
    otime_centis_ = StringToInt(command.arguments.at(0));
    break;

  case USERMOVE: {
    if (player_ == nullptr) {
      ReBuildPlayer(rand_moves_);
    }
    StopPondering();
    Move move(command.arguments.at(0));
    if (force_mode_) {
      std::cout << "# Forced: " << player_->GetBoard()->ParseIntoFEN() << "|"
                << move.str() << std::endl;
      player_->GetBoard()->MakeMove(move);
      OutputFEN();
      break;
    }
    if (!player_builder_->GetMoveGenerator()->IsValidMove(move)) {
      response->push_back("Illegal move: " + move.str());
      break;
    }
    player_->GetBoard()->MakeMove(move);
    OutputFEN();
    if (MatchResult(response)) {
      break;
    }

    Move cmove = player_->Search(search_params_, AllocateTime());
    player_->GetBoard()->MakeMove(cmove);
    OutputFEN();
    response->push_back("move " + cmove.str());
    if (MatchResult(response)) {
      break;
    }
    StartPondering(time_centis_);
  } break;

  case FORCE:
    StopPondering();
    force_mode_ = true;
    break;

  case SHOWBOARD:
    player_->GetBoard()->DebugPrintBoard();
    break;

  case UNMAKE:
    player_->GetBoard()->UnmakeLastMove();
    break;

  case MOVELIST: {
    MoveGenerator* movegen = nullptr;
    switch (variant_) {
    case Variant::STANDARD:
      movegen = new MoveGeneratorStandard(player_->GetBoard());
      break;
    case Variant::ANTICHESS:
      movegen = new MoveGeneratorAntichess(*player_->GetBoard());
      break;
    }
    MoveArray move_array;
    movegen->GenerateMoves(&move_array);
    for (size_t i = 0; i < move_array.size(); ++i) {
      std::cout << "# " << i + 1 << " " << move_array.get(i).str() << std::endl;
    }
    delete movegen;
  } break;

  case NOBOOK:
    book_ = false;
    ReBuildPlayer(rand_moves_);
    break;

  case NOPNS:
    pns_ = false;
    ReBuildPlayer(rand_moves_);
    break;

  case NOPONDER:
    ponder_ = false;
    break;

  case NOPOST:
    search_params_.thinking_output = false;
    break;

  case PING:
    response->push_back("pong " + command.arguments.at(0));
    break;

  case POST:
    search_params_.thinking_output = true;
    break;

  case RANDMOVES:
    rand_moves_ = StringToInt(command.arguments.at(0));
    ReBuildPlayer(rand_moves_);
    break;

  case QUIT:
    quit_ = true;
    break;

  // Ignore
  case ERROR:
  case FEATURE:
    break;

  default:
    response->push_back("Error (Unknown command): " + command_str);
    break;
  }
  if (quit_ && player_builder_.get()) {
    StopPondering();
    player_builder_->GetTranspos()->LogStats();
    if (player_builder_->GetEGTB()) {
      player_builder_->GetEGTB()->LogStats();
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
  std::cout << "# FEN: " << player_->GetBoard()->ParseIntoFEN() << std::endl;
}
