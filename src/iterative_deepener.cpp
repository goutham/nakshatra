#include "iterative_deepener.h"
#include "board.h"
#include "common.h"
#include "config.h"
#include "egtb.h"
#include "eval.h"
#include "eval_antichess.h"
#include "eval_standard.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "san.h"
#include "search_algorithm.h"
#include "stats.h"
#include "stopwatch.h"
#include "timer.h"
#include "transpos.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

void IterativeDeepener::Search(const IDSParams& ids_params, Move* best_move,
                               int* best_move_score,
                               SearchStats* id_search_stats) {
  std::ostream& out = ids_params.thinking_output ? std::cout : nullstream;
  StopWatch stop_watch;
  stop_watch.Start();

  movegen_->GenerateMoves(&root_move_array_);
  out << "# Number of moves at root: " << root_move_array_.size() << std::endl;

  // No moves to make. Just return by setting invalid move. This can happen if
  // Search() is called after game ends.
  if (root_move_array_.size() == 0) {
    *best_move = Move();
    *best_move_score = INF;
    return;
  }

  // Caller provided move ordering and pruning takes priority over move orderer.
  if (ids_params.pruned_ordered_moves.size()) {
    root_move_array_ = ids_params.pruned_ordered_moves;
  } else {
    MoveInfoArray move_info_array;
    move_orderer_->Order(root_move_array_, nullptr, &move_info_array);
    root_move_array_.clear();
    for (size_t i = 0; i < move_info_array.size; ++i) {
      root_move_array_.Add(move_info_array.moves[i].move);
    }
  }
  assert(root_move_array_.size() > 0);
  out << "# Number of root moves being searched: " << root_move_array_.size()
      << std::endl;

  // If there is only one move to be made, make it without hesitation as search
  // won't yield anything new.
  if (root_move_array_.size() == 1) {
    *best_move = root_move_array_.get(0);
    *best_move_score = INF;
    return;
  }

  // Iterative deepening starts here.
  for (unsigned depth = 1; depth <= ids_params.search_depth; ++depth) {
    if (!iteration_stats_.empty()) {
      auto& move_stats = iteration_stats_.back().move_stats;
      // A common technique for root move ordering: sort moves by size of their
      // search trees in previous iteration (largest to smallest). The heuristic
      // being that moves with larger search trees are harder to refute.
      std::sort(move_stats.begin(), move_stats.end(),
                [](const std::pair<Move, SearchStats>& a,
                   const std::pair<Move, SearchStats>& b) -> bool {
                  return a.second.nodes_searched > b.second.nodes_searched;
                });
      root_move_array_.clear();
      for (const auto& stat : move_stats) {
        root_move_array_.Add(stat.first);
      }
      root_move_array_.PushToFront(iteration_stats_.back().best_move);
    } else if (ids_params.pruned_ordered_moves.size() == 0) {
      bool found = false;
      const TTData tdata = transpos_->Get(board_->ZobristKey(), &found);
      if (found && tdata.best_move.is_valid()) {
        root_move_array_.PushToFront(tdata.best_move);
      }
    }

    const auto istat = FindBestMove(depth);
    iteration_stats_.push_back(istat);

    double elapsed_time = stop_watch.ElapsedTime();

    const IterationStat& last_istat = iteration_stats_.back();

    // If FindMove could not complete at least the first root move subtree
    // completely, don't report stats or update best_move as the results are
    // likely to be incorrect.
    if (timer_->Lapsed() && last_istat.root_moves_covered == 0) {
      break;
    }

    // Update best_move and stats.
    *best_move = last_istat.best_move;
    *best_move_score = last_istat.score;
    for (const auto& stat : last_istat.move_stats) {
      id_search_stats->nodes_searched += stat.second.nodes_searched;
      id_search_stats->search_depth = stat.second.search_depth;
    }

    // XBoard style thinking output.
    if (ids_params.thinking_output) {
      char output[256];
      snprintf(output, 256, "%2d\t%5d\t%5d\t%10lu\t%s", depth, last_istat.score,
               int(elapsed_time), id_search_stats->nodes_searched,
               PV(*best_move).c_str());
      std::cout << output << std::endl;
    }

    // Don't go any deeper if a win is confirmed or timer has lapsed.
    if (last_istat.score == WIN || timer_->Lapsed()) {
      break;
    }

// Lower accuracy but saves time on "easy" moves.
#ifdef SAVETIME
    if (variant_ == Variant::STANDARD) {
      if (depth >= 8) {
        int iters = 0;
        for (auto it = iteration_stats_.rbegin(); it != iteration_stats_.rend();
             ++it) {
          if (it->best_move == *best_move) {
            ++iters;
          } else {
            break;
          }
        }
        if ((depth >= 8 && iters >= 5) || (depth >= 10 && iters >= 3)) {
          break;
        }
      }
    }
#endif
  }
  stop_watch.Stop();
  out << "# Time taken for ID search: " << stop_watch.ElapsedTime() << " centis"
      << std::endl;
}

// Finds the best move by searching up to given max_depth. Stops and returns
// quickly if timer expires during computation. Updates iteration_stats_ with
// details of current iteration.
IterativeDeepener::IterationStat
IterativeDeepener::FindBestMove(int max_depth) {

  auto search = [max_depth, root_move_array = root_move_array_,
                 &transpos = transpos_,
                 &timer = timer_](int thread_num, Board* board,
                                  SearchAlgorithm* search_algorithm,
                                  IterationStat* ret_istat) mutable {
    if (thread_num % 2 == 1) {
      ++max_depth;
    }
    IterationStat istat;
    istat.depth = max_depth;
    istat.best_move = root_move_array.get(0);
    istat.score = -INF;
    istat.root_moves_covered = 0;
    for (unsigned int i = 0; i < root_move_array.size(); ++i) {
      const Move& move = root_move_array.get(i);
      board->MakeMove(move);
      SearchStats search_stats;
      int score = -INF;
      if (i == 0 || max_depth < 5) {
        score = -search_algorithm->Search(max_depth - 1, -INF, -istat.score,
                                          &search_stats);
      } else {
        bool lmr_triggered = false;
        if (i >= 4 && max_depth >= 2) {
          score = -search_algorithm->Search(max_depth - 2, -istat.score - 1,
                                            -istat.score, &search_stats);
          lmr_triggered = true;
        }
        if (!lmr_triggered || score > istat.score) {
          score = -search_algorithm->Search(max_depth - 1, -istat.score - 1,
                                            -istat.score, &search_stats);
        }
        if (score > istat.score) {
          score = -search_algorithm->Search(max_depth - 1, -INF, -istat.score,
                                            &search_stats);
        }
      }
      board->UnmakeLastMove();
      istat.move_stats.push_back(std::make_pair(move, search_stats));

      // Return on timer expiry only if we are not searching at depth 1. If
      // searching at depth 1, we should at least quickly find a meaningful
      // move even if timer expires before all the root moves are evaluated.
      // Without this, there is a possibility of iterative deepener not
      // reporting any moves at all in some extremely time constrained
      // situations. Searching all root moves at depth 1 is very quick
      // (sub-millisecond latency).
      if (timer->Lapsed() && max_depth > 1) {
        break;
      }
      if (score > istat.score) {
        istat.best_move = move;
        istat.score = score;
      }
      ++istat.root_moves_covered;
    }
    // Add move to transposition table if at least the first root move was
    // completely searched to current depth before timer lapsed. Otherwise,
    // we don't really have any valid move to update. Due to move ordering
    // guarantees, the first move in root_move_array_ is guaranteed to be
    // the best known move before current iteration, which means any other
    // move found to be better at this depth is at least better than that.
    if (istat.root_moves_covered > 0) {
      transpos->Put(istat.score, EXACT_NODE, max_depth, board->ZobristKey(),
                    istat.best_move);
    }
    *ret_istat = istat;
  };

  struct Context {
    std::unique_ptr<Board> board;
    std::unique_ptr<MoveGenerator> movegen;
    std::unique_ptr<Evaluator> eval;
    std::unique_ptr<MoveOrderer> move_orderer;
    std::unique_ptr<SearchAlgorithm> search_algorithm;
    IterationStat istat;
  };

  bool abort = false;
  std::vector<Context> contexts;
  const int num_threads = (max_depth < 3 ? 1 : NUM_THREADS);
  for (int i = 0; i < num_threads; ++i) {
    Context ctxt;
    ctxt.board = std::make_unique<Board>(*board_);
    if (variant_ == Variant::STANDARD) {
      ctxt.movegen = std::make_unique<MoveGeneratorStandard>(ctxt.board.get());
      ctxt.move_orderer =
          std::make_unique<StandardMoveOrderer>(ctxt.board.get());
      ctxt.eval =
          std::make_unique<EvalStandard>(ctxt.board.get(), ctxt.movegen.get());
    } else if (variant_ == Variant::ANTICHESS) {
      ctxt.movegen = std::make_unique<MoveGeneratorAntichess>(*ctxt.board);
      ctxt.move_orderer = std::make_unique<AntichessMoveOrderer>(
          ctxt.board.get(), ctxt.movegen.get());
      ctxt.eval = std::make_unique<EvalAntichess>(ctxt.board.get(),
                                                  ctxt.movegen.get(), egtb_);
    }
    ctxt.search_algorithm = std::make_unique<SearchAlgorithm>(
        variant_, ctxt.board.get(), ctxt.movegen.get(), ctxt.eval.get(), timer_,
        transpos_, ctxt.move_orderer.get(), i == 0 ? nullptr : &abort);
    contexts.push_back(std::move(ctxt));
  }

  std::vector<std::thread> threads;
  for (size_t i = 1; i < contexts.size(); ++i) {
    auto& ctxt = contexts.at(i);
    threads.push_back(std::thread(search, i, ctxt.board.get(),
                                  ctxt.search_algorithm.get(), &ctxt.istat));
  }

  auto& ctxt = contexts.at(0);
  search(0, ctxt.board.get(), ctxt.search_algorithm.get(), &ctxt.istat);
  abort = true;

  for (auto& thread : threads) {
    thread.join();
  }

  for (size_t i = 1; i < contexts.size(); ++i) {
    ctxt.istat.MergeStats(contexts.at(i).istat);
  }
  return ctxt.istat;
}

std::string IterativeDeepener::PV(const Move& root_move) {
  std::string pv;

  // Root move may not always be in the transposition table or the one in
  // transposition table may not be the first move of PV if move ordering
  // is determined externally.
  pv.append(SAN(*board_, root_move) + " ");
  board_->MakeMove(root_move);

  int depth = 0;
  while (depth < 10) {
    U64 zkey = board_->ZobristKey();
    bool found = false;
    const TTData tdata = transpos_->Get(zkey, &found);
    if (!found || !tdata.best_move.is_valid() ||
        tdata.node_type() != EXACT_NODE) {
      break;
    }
    pv.append(SAN(*board_, tdata.best_move) + " ");
    ++depth;
    board_->MakeMove(tdata.best_move);
  }
  while (depth--) {
    board_->UnmakeLastMove();
  }

  board_->UnmakeLastMove(); // root move
  return pv;
}
