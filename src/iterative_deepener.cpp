#include "iterative_deepener.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "extensions.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "san.h"
#include "search_algorithm.h"
#include "stats.h"
#include "stopwatch.h"
#include "timer.h"
#include "transpos.h"

#include <cstring>
#include <iostream>
#include <string>

void IterativeDeepener::Search(const IDSParams& ids_params, Move* best_move,
                               int* best_move_score,
                               SearchStats* id_search_stats) {
  std::ostream& out = ids_params.thinking_output ? std::cout : nullstream;
  ClearState();

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
  } else if (extensions_->move_orderer) {
    extensions_->move_orderer->Order(&root_move_array_);
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
    // Do not use transposition table moves at the root if ordered/pruned
    // movelist is passed by the caller as we expect input ordering to be of
    // highest quality. Also, this avoids transposition table moves that are
    // not in the list to be brought to the front.
    if (ids_params.pruned_ordered_moves.size() == 0) {
      TranspositionTableEntry* tentry = transpos_->Get(board_->ZobristKey());
      if (tentry && tentry->best_move.is_valid()) {
        root_move_array_.PushToFront(tentry->best_move);
      }
    } else if (!iteration_stats_.empty()) {
      root_move_array_.PushToFront(iteration_stats_.back().best_move);
    }

    // This will return immediately when timer expires but updates the
    // iteration_stats_ nevertheless. Results from the last iteration are
    // acceptable only if search of at least the first root move subtree was
    // completed. Due to the move-ordering done above, the first root move that
    // is searched is guaranteed to be the best known move before beginning of
    // this iteration. So, if a different best move is found before timer
    // expiry, it is at least better than the previously known best move
    // (though it might not be the overall best move at this depth because all
    // the root moves might not be covered).
    FindBestMove(depth);

    double elapsed_time = stop_watch.ElapsedTime();

    const IterationStat& last_istat = iteration_stats_.back();

    // If FindMove could not complete at least the first root move subtree
    // completely, don't report stats or update best_move as the results are
    // likely to be incorrect.
    if (timer_->timer_expired() && last_istat.root_moves_covered == 0) {
      break;
    }

    // Update best_move and stats.
    *best_move = last_istat.best_move;
    *best_move_score = last_istat.score;
    id_search_stats->nodes_searched += last_istat.search_stats.nodes_searched;
    id_search_stats->nodes_researched +=
        last_istat.search_stats.nodes_researched;
    id_search_stats->nodes_evaluated += last_istat.search_stats.nodes_evaluated;
    id_search_stats->search_depth = last_istat.depth;

    // XBoard style thinking output.
    if (ids_params.thinking_output) {
      char output[256];
      snprintf(output, 256, "%2d\t%5d\t%5d\t%10d\t%s", depth, last_istat.score,
               int(elapsed_time), id_search_stats->nodes_evaluated,
               PV(*best_move).c_str());
      std::cout << output << std::endl;
    }

    // Don't go any deeper if a win is confirmed or timer has expired.
    if (last_istat.score == WIN || timer_->timer_expired()) {
      break;
    }
  }
  stop_watch.Stop();
  out << "# Time taken for ID search: " << stop_watch.ElapsedTime() << " centis"
      << std::endl;
}

// Finds the best move by searching up to given max_depth. Stops and returns
// quickly if timer expires during computation. Updates iteration_stats_ with
// details of current iteration.
void IterativeDeepener::FindBestMove(int max_depth) {
  IterationStat istat;
  istat.depth = max_depth;
  istat.best_move = root_move_array_.get(0);
  istat.score = -INF;
  istat.root_moves_covered = 0;
  for (unsigned int i = 0; i < root_move_array_.size(); ++i) {
    const Move& move = root_move_array_.get(i);
    board_->MakeMove(move);
    int score = -search_algorithm_->NegaScout(max_depth - 1, -INF, INF,
                                              &istat.search_stats);
    board_->UnmakeLastMove();

    // Return on timer expiry only if we are not searching at depth 1. If
    // searching at depth 1, we should at least quickly find a meaningful move
    // even if timer expires before all the root moves are evaluated. Without
    // this, there is a possibility of iterative deepener not reporting any
    // moves at all in some extremely time constrained situations. Searching all
    // root moves at depth 1 is very quick (sub-millisecond latency).
    if (timer_->timer_expired() && max_depth > 1) {
      break;
    }
    if (score > istat.score) {
      istat.best_move = move;
      istat.score = score;
    }
    ++istat.root_moves_covered;
  }
  // Add move to transposition table if at least the first root move was
  // completely searched to current depth before timer expired. Otherwise, we
  // don't really have any valid move to update. Due to move ordering
  // guarantees, the first move in root_move_array_ is guaranteed to be the
  // best known move before current iteration, which means any other move found
  // to be better at this depth is at least better than that.
  if (istat.root_moves_covered > 0) {
    transpos_->Put(istat.score, EXACT_NODE, max_depth, board_->ZobristKey(),
                   istat.best_move);
  }
  iteration_stats_.push_back(istat);
}

void IterativeDeepener::ClearState() {
  root_move_array_.clear();
  iteration_stats_.clear();
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
    TranspositionTableEntry* tentry = transpos_->Get(zkey);
    if (!tentry || !tentry->best_move.is_valid() ||
        tentry->node_type != EXACT_NODE) {
      break;
    }
    pv.append(SAN(*board_, tentry->best_move) + " ");
    ++depth;
    board_->MakeMove(tentry->best_move);
  }
  while (depth--) {
    board_->UnmakeLastMove();
  }

  board_->UnmakeLastMove(); // root move
  return pv;
}
