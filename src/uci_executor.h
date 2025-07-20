#ifndef UCI_EXECUTOR_H
#define UCI_EXECUTOR_H

#include "common.h"
#include "board.h"
#include "player.h"
#include "timer.h"
#include "transpos.h"
#include "movegen.h"
#include "move_array.h"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <iostream>
#include <sstream>

class UCIExecutor {
public:
  UCIExecutor();
  ~UCIExecutor();

  // Execute UCI command and return response strings
  std::vector<std::string> Execute(const std::string& command_str);

  // Returns true if program should quit
  bool quit() const { return quit_; }

private:
  bool quit_ = false;
  Variant variant_ = Variant::STANDARD;
  std::unique_ptr<Board> board_;
  std::unique_ptr<TranspositionTable> transpos_;
  std::unique_ptr<Timer> timer_;
  std::unique_ptr<Player> player_;
  std::atomic<bool> searching_{false};
  std::atomic<bool> pondering_{false};
  std::thread search_thread_;
  std::thread ponder_thread_;
  
  // UCI Options
  int hash_size_mb_ = 1;  // Default 1MB transposition table
  std::string uci_variant_ = "standard";  // Default variant
  bool uci_info_output_ = true;  // Enable UCI info output by default
  bool ponder_ = false;  // Pondering (thinking on opponent's time)
  bool analyse_mode_ = false;  // Analysis mode
  bool pns_enabled_ = true;  // Proof Number Search for antichess
  
  // Individual command handlers
  std::vector<std::string> HandleUCI();
  std::vector<std::string> HandleIsReady();
  std::vector<std::string> HandleQuit();
  std::vector<std::string> HandleUCINewGame();
  std::vector<std::string> HandlePosition(const std::vector<std::string>& tokens);
  std::vector<std::string> HandleGo(const std::vector<std::string>& tokens);
  std::vector<std::string> HandleStop();
  std::vector<std::string> HandlePonderHit();
  std::vector<std::string> HandleSetOption(const std::vector<std::string>& tokens);
  
  // Helper methods
  void ResetBoard();
  void InitializeSearchComponents();
  void SearchAndOutput(bool infinite = true, int movetime_ms = 0, int depth = 0);
};

#endif