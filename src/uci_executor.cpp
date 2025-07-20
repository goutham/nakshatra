#include "uci_executor.h"
#include "common.h"

#include <iostream>
#include <sstream>

UCIExecutor::UCIExecutor() {
  InitializeSearchComponents();
  ResetBoard();
}

UCIExecutor::~UCIExecutor() {
  // Always try to stop any running search and join threads
  if (timer_) {
    timer_->Invalidate();
  }
  if (search_thread_.joinable()) {
    search_thread_.join();
  }
  if (ponder_thread_.joinable()) {
    ponder_thread_.join();
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
  } else if (command == "ponderhit") {
    return HandlePonderHit();
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
  response.push_back("option name UCI_Variant type combo default standard var standard var suicide var giveaway");
  response.push_back("option name Ponder type check default false");
  response.push_back("option name UCI_AnalyseMode type check default false");
  response.push_back("option name PNS type check default true");
  response.push_back("option name Threads type spin default 1 min 1 max 8");
  response.push_back("option name MultiPV type spin default 1 min 1 max 5");
  response.push_back("option name UCI_ShowCurrLine type check default false");
  response.push_back("option name UCI_ShowRefutations type check default false");
  response.push_back("option name UCI_LimitStrength type check default false");
  response.push_back("option name UCI_Elo type spin default 2850 min 1000 max 3000");
  
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
      // Reset position history for new FEN position
      position_history_.clear();
      UpdatePositionHistory();
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
          // Validate move is legal in current position
          MoveArray legal_moves;
          if (variant_ == Variant::STANDARD) {
            legal_moves = GenerateMoves<Variant::STANDARD>(*board_);
          } else if (variant_ == Variant::ANTICHESS) {
            legal_moves = GenerateMoves<Variant::ANTICHESS>(*board_);
          } else if (variant_ == Variant::SUICIDE) {
            legal_moves = GenerateMoves<Variant::SUICIDE>(*board_);
          }
          
          bool is_legal = false;
          for (size_t j = 0; j < legal_moves.size(); ++j) {
            if (legal_moves.get(j) == move) {
              is_legal = true;
              break;
            }
          }
          
          if (is_legal) {
            board_->MakeMove(move);
            UpdatePositionHistory();
          } else {
            // Illegal move - stop processing
            break;
          }
        } else {
          // Invalid move format - stop processing
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
  bool ponder = false;
  int movetime_ms = 0;
  int depth = 0;
  int mate = 0;
  int wtime_ms = 0;
  int btime_ms = 0;
  int winc_ms = 0;
  int binc_ms = 0;
  int movestogo = 0;
  long nodes = 0;
  
  for (size_t i = 1; i < tokens.size(); ++i) {
    if (tokens[i] == "infinite") {
      infinite = true;
    } else if (tokens[i] == "ponder") {
      ponder = true;
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
    } else if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
      try {
        wtime_ms = std::stoi(tokens[i + 1]);
        ++i;  // Skip the wtime value
      } catch (const std::exception& e) {
        // Invalid wtime value - ignore
      }
    } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
      try {
        btime_ms = std::stoi(tokens[i + 1]);
        ++i;  // Skip the btime value
      } catch (const std::exception& e) {
        // Invalid btime value - ignore
      }
    } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
      try {
        winc_ms = std::stoi(tokens[i + 1]);
        ++i;  // Skip the winc value
      } catch (const std::exception& e) {
        // Invalid winc value - ignore
      }
    } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
      try {
        binc_ms = std::stoi(tokens[i + 1]);
        ++i;  // Skip the binc value
      } catch (const std::exception& e) {
        // Invalid binc value - ignore
      }
    } else if (tokens[i] == "movestogo" && i + 1 < tokens.size()) {
      try {
        movestogo = std::stoi(tokens[i + 1]);
        ++i;  // Skip the movestogo value
      } catch (const std::exception& e) {
        // Invalid movestogo value - ignore
      }
    } else if (tokens[i] == "nodes" && i + 1 < tokens.size()) {
      try {
        nodes = std::stol(tokens[i + 1]);
        ++i;  // Skip the nodes value
      } catch (const std::exception& e) {
        // Invalid nodes value - ignore
      }
    } else if (tokens[i] == "mate" && i + 1 < tokens.size()) {
      try {
        mate = std::stoi(tokens[i + 1]);
        ++i;  // Skip the mate value
      } catch (const std::exception& e) {
        // Invalid mate value - ignore
      }
    }
  }
  
  // Calculate time for this move from time control parameters
  int calculated_time_ms = 0;
  if (wtime_ms > 0 || btime_ms > 0) {
    // Determine which color we are (assume we're white if no board state info)
    // For simplicity, use white time for now - real implementation would check current side
    int our_time_ms = wtime_ms > 0 ? wtime_ms : btime_ms;
    int our_inc_ms = wtime_ms > 0 ? winc_ms : binc_ms;
    
    // Simple time management: use 1/30 of remaining time plus increment
    calculated_time_ms = our_time_ms / 30 + our_inc_ms;
    
    // Minimum 100ms, maximum 1/3 of remaining time
    calculated_time_ms = std::max(100, std::min(calculated_time_ms, our_time_ms / 3));
  }
  
  // If no valid search parameters, ignore (but allow ponder and mate)
  if (!infinite && !ponder && movetime_ms <= 0 && depth <= 0 && calculated_time_ms <= 0 && nodes <= 0 && mate <= 0) {
    return {};
  }
  
  if (ponder && ponder_) {
    // Start pondering if enabled
    pondering_ = true;
    ponder_thread_ = std::thread(&UCIExecutor::PonderAndOutput, this, depth > 0 ? depth : MAX_DEPTH);
  } else {
    // Regular search
    // Determine final search time - prefer explicit movetime, then calculated time
    int final_time_ms = movetime_ms > 0 ? movetime_ms : calculated_time_ms;
    
    // For mate search, set deep depth and reasonable time
    if (mate > 0) {
      depth = mate * 2 + 2;  // Mate in N needs at least 2*N plies
      final_time_ms = std::max(final_time_ms, 10000);  // At least 10 seconds for mate search
    }
    
    // Start search in a separate thread
    searching_ = true;
    search_thread_ = std::thread(&UCIExecutor::SearchAndOutput, this, infinite, final_time_ms, depth);
  }
  
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
  if (pondering_) {
    if (ponder_thread_.joinable()) {
      ponder_thread_.join();
    }
    pondering_ = false;
  }
  return {};
}

std::vector<std::string> UCIExecutor::HandlePonderHit() {
  if (pondering_) {
    // Stop pondering - the search becomes a regular search
    pondering_ = false;
    // The ponder thread will detect this and output bestmove immediately
  }
  return {};
}

void UCIExecutor::ResetBoard() {
  board_ = std::make_unique<Board>(variant_);
  player_ = std::make_unique<Player>(variant_, *board_, *transpos_, *timer_);
  
  // Reset game state tracking
  position_history_.clear();
  fifty_move_rule_counter_ = 0;
  UpdatePositionHistory();
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
    
    // Check for draw before searching
    if (IsDrawPosition()) {
      std::cout << "info string Draw detected: ";
      if (IsRepetitionDraw()) {
        std::cout << "threefold repetition" << std::endl;
      } else if (IsFiftyMoveRuleDraw()) {
        std::cout << "fifty-move rule" << std::endl;
      }
      std::cout << "bestmove 0000" << std::endl;  // No best move in draw position
      return;
    }
    
    SearchParams params;
    params.thinking_output = uci_info_output_;  // Enable UCI info output
    params.uci_output_format = true;           // Use UCI info format
    params.search_depth = search_depth;
    params.antichess_pns = pns_enabled_ && (variant_ == Variant::ANTICHESS || variant_ == Variant::SUICIDE);  // Enable PNS for antichess variants if option enabled
    
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

void UCIExecutor::PonderAndOutput(int depth) {
  try {
    // Set up pondering search with infinite time but limited depth
    timer_->Run(3600000);  // 1 hour - effectively infinite for pondering
    
    SearchParams params;
    params.thinking_output = uci_info_output_;  // Enable UCI info output
    params.uci_output_format = true;           // Use UCI info format
    params.search_depth = depth;
    params.antichess_pns = pns_enabled_ && (variant_ == Variant::ANTICHESS || variant_ == Variant::SUICIDE);
    
    Move best_move = player_->Search(params, 360000);  // 1 hour in centiseconds
    
    // Only output bestmove if pondering wasn't stopped (ponderhit or stop)
    if (pondering_ && best_move.is_valid()) {
      std::cout << "bestmove " << best_move.str() << std::endl;
    }
  } catch (const std::exception& e) {
    // Handle any exceptions during pondering
    if (pondering_) {
      std::cout << "bestmove 0000" << std::endl;
    }
  } catch (...) {
    // Handle any other exceptions
    if (pondering_) {
      std::cout << "bestmove 0000" << std::endl;
    }
  }
  
  // Always reset pondering flag
  pondering_ = false;
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
    } else if (option_value == "giveaway") {
      variant_ = Variant::ANTICHESS;
      uci_variant_ = "giveaway";
    }
    // Rebuild board and player with new variant
    if (!searching_) {
      ResetBoard();
    }
  } else if (option_name == "Ponder") {
    if (option_value == "true") {
      ponder_ = true;
    } else if (option_value == "false") {
      ponder_ = false;
    }
  } else if (option_name == "UCI_AnalyseMode") {
    if (option_value == "true") {
      analyse_mode_ = true;
    } else if (option_value == "false") {
      analyse_mode_ = false;
    }
  } else if (option_name == "PNS") {
    if (option_value == "true") {
      pns_enabled_ = true;
    } else if (option_value == "false") {
      pns_enabled_ = false;
    }
  } else if (option_name == "Threads") {
    try {
      int threads = std::stoi(option_value);
      if (threads >= 1 && threads <= 8) {
        threads_ = threads;
      }
    } catch (const std::exception& e) {
      // Invalid threads value - ignore
    }
  } else if (option_name == "MultiPV") {
    try {
      int multipv = std::stoi(option_value);
      if (multipv >= 1 && multipv <= 5) {
        multipv_ = multipv;
      }
    } catch (const std::exception& e) {
      // Invalid MultiPV value - ignore
    }
  } else if (option_name == "UCI_ShowCurrLine") {
    if (option_value == "true") {
      show_currline_ = true;
    } else if (option_value == "false") {
      show_currline_ = false;
    }
  } else if (option_name == "UCI_ShowRefutations") {
    if (option_value == "true") {
      show_refutations_ = true;
    } else if (option_value == "false") {
      show_refutations_ = false;
    }
  } else if (option_name == "UCI_LimitStrength") {
    if (option_value == "true") {
      limit_strength_ = true;
    } else if (option_value == "false") {
      limit_strength_ = false;
    }
  } else if (option_name == "UCI_Elo") {
    try {
      int elo = std::stoi(option_value);
      if (elo >= 1000 && elo <= 3000) {
        elo_rating_ = elo;
      }
    } catch (const std::exception& e) {
      // Invalid Elo value - ignore
    }
  }
  
  return {};
}

void UCIExecutor::UpdatePositionHistory() {
  if (board_) {
    U64 current_zobrist = board_->ZobristKey();
    position_history_.push_back(current_zobrist);
    fifty_move_rule_counter_ = board_->HalfMoveClock();
  }
}

bool UCIExecutor::IsRepetitionDraw() const {
  if (position_history_.empty() || !board_) {
    return false;
  }
  
  U64 current_zobrist = board_->ZobristKey();
  int repetition_count = 0;
  
  // Count how many times current position has occurred
  for (U64 zobrist : position_history_) {
    if (zobrist == current_zobrist) {
      repetition_count++;
    }
  }
  
  // Draw if position has occurred 3 times (threefold repetition)
  return repetition_count >= 3;
}

bool UCIExecutor::IsFiftyMoveRuleDraw() const {
  if (!board_) {
    return false;
  }
  
  // Draw if 50 moves (100 half-moves) have passed without pawn move or capture
  return board_->HalfMoveClock() >= 100;
}

bool UCIExecutor::IsDrawPosition() const {
  return IsRepetitionDraw() || IsFiftyMoveRuleDraw();
}