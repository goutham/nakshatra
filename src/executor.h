#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "common.h"
#include "player.h"
#include "transpos.h"

#include <memory>
#include <string>
#include <thread>
#include <vector>

struct ExecutionContext final {
  Variant variant;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<Board> board;
  std::unique_ptr<TranspositionTable> transpos;
  std::unique_ptr<Player> player;
  bool own_transpos = true;

  struct Options {
    // Initial FEN to construct board with. If this is empty, use default
    // initialization for given variant.
    std::string init_fen;

    // Use this transposition table instead of building a new one.
    TranspositionTable* transpos = nullptr;
  };

  ExecutionContext(const Variant variant, const Options& options);
  ~ExecutionContext();
};

class Executor {
public:
  Executor(const std::string& name) : Executor(name, "", Variant::STANDARD) {}

  Executor(const std::string& name, const std::string& init_fen,
           const Variant variant)
      : name_(name), variant_(variant), time_centis_(10 * 60 * 100),
        otime_centis_(10 * 60 * 100), init_fen_(init_fen) {}

  ~Executor() { StopPondering(); }

  // Executes command. Response may be set (depending on the command) in the
  // response string vector.
  std::vector<std::string> Execute(const std::string& command_str);

  // Returns true if program has to quit.
  bool quit() { return quit_; }

private:
  // Checks if match has a result - i.e, the result code obtained from evaluator
  // is one of WIN, -WIN or DRAW. If a result is available, returns true, else
  // false.
  bool MatchResult(std::vector<std::string>& response);

  void RebuildMainContext();
  void RebuildPonderingContext();

  void StartPondering(double time_centis);
  void StopPondering();

  void OutputFEN() const;

  long AllocateTime() const;

  // Name of the computer player.
  std::string name_;

  SearchParams search_params_;
  std::unique_ptr<ExecutionContext> main_context_;
  std::unique_ptr<ExecutionContext> pondering_context_;
  std::unique_ptr<std::thread> pondering_thread_;
  Variant variant_;
  bool quit_ = false;
  bool force_mode_ = false;
  bool ponder_ = true;
  int think_time_centis_ = -1;

  double time_centis_;
  double otime_centis_;

  // Custom board FEN which is used as initial board for all games if it is non
  // empty.
  std::string init_fen_;
};

#endif
