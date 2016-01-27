#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "common.h"
#include "creation.h"
#include "player.h"

#include <memory>
#include <string>
#include <vector>

class Player;
class TranspositionTable;

class Executor {
 public:
  Executor(const std::string& name)
      : Executor(name, "", Variant::NORMAL) {}

  Executor(const std::string& name,
           const std::string& init_fen,
           const Variant variant)
      : name_(name),
        player_(nullptr),
        variant_(variant),
        time_centis_(10 * 60 * 100),
        otime_centis_(10 * 60 * 100),
        init_fen_(init_fen) {}

  // Executes command and returns true on success. The result string is set or
  // empty depending on the protocol. If error, returns false and result string
  // is set to error message.
  bool Execute(const std::string& command_str,
               std::vector<std::string>* response);

  // Returns true if program has to quit.
  bool quit() { return quit_; }

 private:
  // Checks if match has a result - i.e, the result code obtained from evaluator
  // is one of WIN, -WIN or DRAW. If a result is available, returns true, else
  // false.
  bool MatchResult(std::vector<std::string>* response);

  void ReBuildPlayer();

  void MakeRandomMove(std::vector<std::string>* response);

  void OutputFEN() const;

  // Name of the computer player.
  std::string name_;

  Player* player_;  // not owned.
  SearchParams search_params_;
  std::unique_ptr<PlayerBuilder> player_builder_;
  Variant variant_;
  bool quit_ = false;
  bool force_mode_ = false;

  double time_centis_;
  double otime_centis_;

  // Custom board FEN which is used as initial board for all games if it is non
  // empty.
  std::string init_fen_;
};

#endif
