#include "uci_executor.h"
#include "common.h"

#include <iostream>
#include <sstream>

UCIExecutor::UCIExecutor() {
  InitializeSearchComponents();
  ResetBoard();
}

UCIExecutor::~UCIExecutor() {
  // Always try to stop any running search and join thread
  if (timer_) {
    timer_->Invalidate();
  }
  if (search_thread_.joinable()) {
    search_thread_.join();
  }
}

std::vector<std::string> UCIExecutor::Execute(const std::string& command_str) {
  std::vector<std::string> tokens = SplitString(command_str, ' ');
  if (tokens.empty()) {
    return {};
  }

  const std::string& command = tokens[0];
  
  if (command == "uci") {
    return HandleUCI();
  } else if (command == "isready") {
    return HandleIsReady();
  } else if (command == "quit") {
    return HandleQuit();
  } else if (command == "ucinewgame") {
    return HandleUCINewGame();
  } else if (command == "position") {
    return HandlePosition(tokens);
  } else if (command == "go") {
    return HandleGo(tokens);
  } else if (command == "stop") {
    return HandleStop();
  } else if (command == "setoption") {
    return HandleSetOption(tokens);
  }
  
  // Unknown command - ignore silently per UCI spec
  return {};
}

std::vector<std::string> UCIExecutor::HandleUCI() {
  std::vector<std::string> response;
  
  // Engine identification
  response.push_back("id name " + std::string(ENGINE_NAME));
  response.push_back("id author Goutham Bhat");
  
  // Engine options
  response.push_back("option name Hash type spin default 1 min 1 max 64");
  response.push_back("option name UCI_Variant type combo default standard var standard var suicide");
  
  // Signal end of UCI response
  response.push_back("uciok");
  
  return response;
}

std::vector<std::string> UCIExecutor::HandleIsReady() {
  return {"readyok"};
}

std::vector<std::string> UCIExecutor::HandleQuit() {
  quit_ = true;
  return {};
}

std::vector<std::string> UCIExecutor::HandleUCINewGame() {
  ResetBoard();
  return {};
}

std::vector<std::string> UCIExecutor::HandlePosition(const std::vector<std::string>& tokens) {
  if (tokens.size() < 2) {
    return {};  // Invalid position command
  }
  
  const std::string& position_type = tokens[1];
  size_t moves_index = 0;
  
  if (position_type == "startpos") {
    ResetBoard();
    moves_index = 2;  // Look for moves starting at token 2
  } else if (position_type == "fen") {
    if (tokens.size() < 8) {
      return {};  // FEN requires 6 parts minimum
    }
    
    // Reconstruct FEN string from tokens 2-7
    std::string fen = tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + 
                      tokens[5] + " " + tokens[6] + " " + tokens[7];
    
    try {
      board_ = std::make_unique<Board>(variant_, fen);
      player_ = std::make_unique<Player>(variant_, *board_, *transpos_, *timer_);
      moves_index = 8;  // Look for moves starting at token 8
    } catch (const std::exception& e) {
      // Invalid FEN - ignore silently per UCI spec
      return {};
    }
  } else {
    // Unknown position type - ignore
    return {};
  }
  
  // Handle move list if present
  if (moves_index < tokens.size() && tokens[moves_index] == "moves") {
    for (size_t i = moves_index + 1; i < tokens.size(); ++i) {
      try {
        Move move(tokens[i]);
        if (move.is_valid()) {
          board_->MakeMove(move);
        } else {
          // Invalid move - stop processing
          break;
        }
      } catch (const std::exception& e) {
        // Invalid move format - stop processing
        break;
      }
    }
  }
  
  return {};
}

std::vector<std::string> UCIExecutor::HandleGo(const std::vector<std::string>& tokens) {
  // Always join previous search thread if it exists
  if (search_thread_.joinable()) {
    search_thread_.join();
  }
  
  // Reset searching flag after joining thread
  searching_ = false;
  
  // Parse go command parameters
  bool infinite = false;
  int movetime_ms = 0;
  int depth = 0;
  
  for (size_t i = 1; i < tokens.size(); ++i) {
    if (tokens[i] == "infinite") {
      infinite = true;
    } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
      try {
        movetime_ms = std::stoi(tokens[i + 1]);
        ++i;  // Skip the movetime value
      } catch (const std::exception& e) {
        // Invalid movetime value - ignore
      }
    } else if (tokens[i] == "depth" && i + 1 < tokens.size()) {
      try {
        depth = std::stoi(tokens[i + 1]);
        ++i;  // Skip the depth value
      } catch (const std::exception& e) {
        // Invalid depth value - ignore
      }
    }
  }
  
  // If no valid search parameters, ignore
  if (!infinite && movetime_ms <= 0 && depth <= 0) {
    return {};
  }
  
  // Start search in a separate thread
  searching_ = true;
  search_thread_ = std::thread(&UCIExecutor::SearchAndOutput, this, infinite, movetime_ms, depth);
  
  return {};
}

std::vector<std::string> UCIExecutor::HandleStop() {
  if (searching_) {
    timer_->Invalidate();
    if (search_thread_.joinable()) {
      search_thread_.join();
    }
    searching_ = false;
  }
  return {};
}

void UCIExecutor::ResetBoard() {
  board_ = std::make_unique<Board>(variant_);
  player_ = std::make_unique<Player>(variant_, *board_, *transpos_, *timer_);
}

void UCIExecutor::InitializeSearchComponents() {
  // Use configurable transposition table size  
  size_t table_size_bytes = static_cast<size_t>(hash_size_mb_) << 20; // Convert MB to bytes
  size_t num_buckets = table_size_bytes / sizeof(TTBucket);
  transpos_ = std::make_unique<TranspositionTable>(num_buckets);
  timer_ = std::make_unique<Timer>();
}

void UCIExecutor::SearchAndOutput(bool infinite, int movetime_ms, int depth) {
  try {
    // Determine search time and depth
    long time_centis = 100;  // Default 1 second
    int search_depth = MAX_DEPTH;
    
    if (!infinite) {
      if (movetime_ms > 0) {
        time_centis = movetime_ms / 10;  // Convert ms to centiseconds
        timer_->Run(movetime_ms);
      } else if (depth > 0) {
        search_depth = depth;
        timer_->Run(60000);  // 60 seconds max for depth search
      }
    } else {
      timer_->Run(1000);  // 1 second for infinite (will be stopped by stop command)
    }
    
    SearchParams params;
    params.thinking_output = uci_info_output_;  // Enable UCI info output
    params.uci_output_format = true;           // Use UCI info format
    params.search_depth = search_depth;
    params.antichess_pns = false;    // Keep PNS disabled for basic functionality
    
    Move best_move = player_->Search(params, time_centis);
    
    if (best_move.is_valid()) {
      std::cout << "bestmove " << best_move.str() << std::endl;
    } else {
      std::cout << "bestmove 0000" << std::endl;  // No move available
    }
  } catch (const std::exception& e) {
    // Handle any exceptions during search
    std::cout << "bestmove 0000" << std::endl;
  } catch (...) {
    // Handle any other exceptions
    std::cout << "bestmove 0000" << std::endl;
  }
  
  // Always reset searching flag
  searching_ = false;
}

std::vector<std::string> UCIExecutor::HandleSetOption(const std::vector<std::string>& tokens) {
  // Expected format: setoption name <option_name> value <option_value>
  if (tokens.size() < 5) {
    return {};  // Invalid setoption command
  }
  
  if (tokens[1] != "name" || tokens[3] != "value") {
    return {};  // Invalid format
  }
  
  const std::string& option_name = tokens[2];
  const std::string& option_value = tokens[4];
  
  if (option_name == "Hash") {
    try {
      int hash_mb = std::stoi(option_value);
      if (hash_mb >= 1 && hash_mb <= 64) {
        hash_size_mb_ = hash_mb;
        // Rebuild transposition table with new size
        if (!searching_) {
          InitializeSearchComponents();
          if (board_) {
            player_ = std::make_unique<Player>(variant_, *board_, *transpos_, *timer_);
          }
        }
      }
    } catch (const std::exception& e) {
      // Invalid hash value - ignore
    }
  } else if (option_name == "UCI_Variant") {
    if (option_value == "standard") {
      variant_ = Variant::STANDARD;
      uci_variant_ = "standard";
    } else if (option_value == "suicide") {
      variant_ = Variant::SUICIDE;
      uci_variant_ = "suicide";
    }
    // Rebuild board and player with new variant
    if (!searching_) {
      ResetBoard();
    }
  }
  
  return {};
}