#include "common.h"
#include "board.h"
#include "creation.h"
#include "executor.h"
#include "eval.h"
#include "eval_suicide.h"
#include "eval_normal.h"
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
  INVALID,  // if command entered is invalid.
  MOVELIST,
  NEW,
  NOPOST,
  OMOVELIST,
  OTIME,
  PING,
  POST,
  QUIT,
  RESULT,
  SEARCH_DEPTH,
  SETBOARD,
  SHOWBOARD,
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
    {"Error", ERROR},
    {"feature", FEATURE},
    {"force", FORCE},
    {"go", GO},
    {"move", USERMOVE},
    {"mlist", MOVELIST},
    {"omlist", OMOVELIST},
    {"new", NEW},
    {"nopost", NOPOST},
    {"otim", OTIME},
    {"ping", PING},
    {"post", POST},
    {"quit", QUIT},
    {"result", RESULT},
    {"sd", SEARCH_DEPTH},
    {"setboard", SETBOARD},
    {"sb", SHOWBOARD},
    {"time", TIME},
    {"usermove", USERMOVE},
    {"variant", VARIANT},
    {"unmake", UNMAKE}
  };
  std::vector<std::string> parts = SplitString(cmd, ' ');
  Command command;
  auto cmd_map_kv = cmd_map.find(parts[0]);
  if (cmd_map_kv == cmd_map.end()) {
    command.cmd_name = INVALID;
  } else {
    command.cmd_name = cmd_map_kv->second;
  }
  for (size_t i = 1; i < parts.size(); ++i) {
    command.arguments.push_back(parts[i]);
  }
  return command;
}

}  // namespace


bool Executor::MatchResult(vector<string>* response) {
  int result = player_builder_->GetEvaluator()->Result();
  if (!(result == WIN || result == -WIN || result == DRAW)) {
    return false;
  }
  string match_result;
  if (result == WIN || result == -WIN) {
    if (player_->GetBoard()->SideToMove() == Side::WHITE) {
      match_result = "1-0 {White Wins}";
    } else {
      match_result = "0-1 {Black Wins}";
    }
  } else {
    match_result = "1/2-1/2 {Draw}";
  }
  response->push_back(match_result);
  return true;
}

void Executor::ReBuildPlayer() {
  switch (variant_) {
    case Variant::NORMAL:
      player_builder_.reset(new NormalPlayerBuilder());
      break;
    case Variant::SUICIDE:
      player_builder_.reset(new SuicidePlayerBuilder());
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
    case Variant::NORMAL:
      ponderer_builder_.reset(new NormalPlayerBuilder());
      break;
    case Variant::SUICIDE:
      ponderer_builder_.reset(new SuicidePlayerBuilder(false));
      break;
  }
  assert(ponderer_builder_.get() != nullptr);
  if (!player_) {
    ReBuildPlayer();
  }
  PlayerBuilderDirector director(ponderer_builder_.get());
  BuildOptions options;
  options.init_fen = player_->GetBoard()->ParseIntoFEN();
  options.transpos = player_builder_->GetTranspos();
  options.build_book = false;
  ponderer_ = director.Build(options);
  assert(ponderer_ != nullptr);
}

void Executor::StartPondering() {
  // If already pondering, then no-op.
  if (pondering_thread_.get() != nullptr) {
    return;
  }
  ReBuildPonderer();
  pondering_thread_.reset(new std::thread([this] {
    // Ponder until stopped or no more moves can be made.
    while (true) {
      SearchParams ponder_params;
      ponder_params.thinking_output = false;
      const Move move = this->ponderer_->Search(ponder_params, 100000, 100000);
      if (move.is_valid() &&
          !this->ponderer_builder_->GetTimer()->timer_expired()) {
        this->ponderer_->GetBoard()->MakeMove(move);
      } else {
        break;
      }
    }
  }));
}

void Executor::StopPondering() {
  // If already not pondering, then no-op.
  if (pondering_thread_.get() == nullptr) {
    return;
  }
  ponderer_builder_->GetTimer()->Stop();
  pondering_thread_->join();
  pondering_thread_.reset(nullptr);
}

void Executor::Execute(const string& command_str,
                       vector<string>* response) {
  Command command = Interpret(command_str);
  switch (command.cmd_name) {
    case NEW:
      {
        StopPondering();
        variant_ = Variant::NORMAL;
        force_mode_ = false;
        search_params_.search_depth = MAX_DEPTH;
        ReBuildPlayer();
      }
      break;

    case VARIANT:
      {
        StopPondering();
        force_mode_ = false;
        variant_ = Variant::NORMAL;
        if (command.arguments.at(0) == "suicide" ||
            command.arguments.at(0) == "S") {
          variant_ = Variant::SUICIDE;
        }
        ReBuildPlayer();
      }
      break;

    case SEARCH_DEPTH:
      {
        std::stringstream ss(command.arguments.at(0));
        ss >> search_params_.search_depth;
        std::cout << "# Search Depth = " << search_params_.search_depth
                  << std::endl;
      }
      break;

    case SETBOARD:
      {
        init_fen_.clear();
        for (const string& part : command.arguments) {
          init_fen_ += part + " ";
        }
        // Erase last character which is a trailing space.
        if (!init_fen_.empty()) {
          init_fen_.erase(init_fen_.end() - 1);
        }
        ReBuildPlayer();
      }
      break;

    case GO:
      {
        StopPondering();
        if (MatchResult(response)) {
          break;
        }
        force_mode_ = false;
        Move cmove =
            player_->Search(search_params_, time_centis_, otime_centis_);
        player_->GetBoard()->MakeMove(cmove);
        OutputFEN();
        response->push_back("move " + cmove.str());
        StartPondering();
      }
      break;

    case TIME:
      time_centis_ = StringToInt(command.arguments.at(0));
      break;

    case OTIME:
      otime_centis_ = StringToInt(command.arguments.at(0));
      break;

    case USERMOVE:
      {
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

        Move cmove =
            player_->Search(search_params_, time_centis_, otime_centis_);
        player_->GetBoard()->MakeMove(cmove);
        OutputFEN();
        response->push_back("move " + cmove.str());
        if (MatchResult(response)) {
          break;
        }
        StartPondering();
      }
      break;

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

    case MOVELIST:
      {
        MoveGenerator* movegen = nullptr;
        switch (variant_) {
          case Variant::NORMAL:
            movegen = new MoveGeneratorNormal(player_->GetBoard());
            break;
          case Variant::SUICIDE:
            movegen = new MoveGeneratorSuicide(*player_->GetBoard());
            break;
        }
        MoveArray move_array;
        movegen->GenerateMoves(&move_array);
        for (size_t i = 0; i < move_array.size(); ++i) {
          std::cout << "# " << i + 1 << " " << move_array.get(i).str()
                    << std::endl;
        }
        delete movegen;
      }
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

void Executor::OutputFEN() const {
  std::cout << "# FEN: " << player_->GetBoard()->ParseIntoFEN() << std::endl;
}
