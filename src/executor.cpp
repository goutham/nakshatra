#include "cmd_interpreter.h"
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
#include <string>
#include <vector>

using std::string;
using std::vector;

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
  assert(player_builder_.get() != NULL);
  PlayerBuilderDirector director(player_builder_.get());
  BuildOptions options;
  options.init_fen = init_fen_;
  player_ = director.Build(options);
  assert(player_ != NULL);
}

bool Executor::Execute(const string& command_str,
                       vector<string>* response) {
  Command command = Interpret(command_str);
  switch (command.cmd_name) {
    case NEW:
      {
        variant_ = Variant::NORMAL;
        force_mode_ = false;
        ReBuildPlayer();
      }
      break;

    case VARIANT:
      {
        force_mode_ = false;
        variant_ = Variant::NORMAL;
        if (command.arguments.at(0) == "suicide" ||
            command.arguments.at(0) == "S") {
          variant_ = Variant::SUICIDE;
        }
        ReBuildPlayer();
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
        if (MatchResult(response)) {
          break;
        }
        force_mode_ = false;
        Move cmove =
            player_->Search(search_params_, time_centis_, otime_centis_);
        player_->GetBoard()->MakeMove(cmove);
        OutputFEN();
        response->push_back("move " + cmove.str());
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
      }
      break;

    case FORCE:
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
        MoveGenerator* movegen;
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
        for (unsigned i = 0; i < move_array.size(); ++i) {
          std::cout << "# " << i + 1 << " " << move_array.get(i).str() << std::endl;
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
    player_builder_->GetTranspos()->LogStats();
    if (player_builder_->GetEGTB()) {
      player_builder_->GetEGTB()->LogStats();
    }
  }
}

void Executor::OutputFEN() const {
  std::cout << "# FEN: " << player_->GetBoard()->ParseIntoFEN() << std::endl;
}
