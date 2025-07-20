#include "id_search.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "pv_search.h"
#include "san.h"
#include "stats.h"
#include "stopwatch.h"
#include "timer.h"
#include "transpos.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>

namespace {

// Stat associated with each iteration of the iterative deepening search
// is stored in the corresponding IterationStat object and pushed in the
// iteration_stats_ vector.
struct IterationStat {
  // Depth of search for current iteration.
  int depth = 0;

  // Best move found in this iteration.
  Move best_move = Move();

  // Score for the best move.
  int score = 0;

  // Number of root moves completely searched before timer
  // expired at current depth.
  int root_moves_covered = 0;

  // Stats for searching to this depth.
  std::vector<std::pair<Move, SearchStats>> move_stats;

  void MergeStats(const IterationStat& istat) {
    std::map<Move, SearchStats> stats_by_move;
    for (const auto& item : istat.move_stats) {
      stats_by_move[item.first] = item.second;
    }
    for (auto& item : move_stats) {
      auto iter = stats_by_move.find(item.first);
      if (iter != stats_by_move.end()) {
        item.second.nodes_searched += iter->second.nodes_searched;
      }
    }
  }
};

template <Variant variant>
class IterativeDeepener {
public:
  IterativeDeepener(const IDSParams& ids_params, Board& board, Timer& timer,
                    TranspositionTable& transpos, EGTB* egtb)
      : ids_params_(ids_params), board_(board), timer_(timer),
        transpos_(transpos), egtb_(egtb) {}

  IDSResult Search();

private:
  IterationStat FindBestMove(int max_depth);

  // Returns principal variation as a string of moves.
  std::string PV(const Move& root_move);

  IDSParams ids_params_;
  Board& board_;
  Timer& timer_;
  TranspositionTable& transpos_;
  EGTB* egtb_;

  // Maintains list of moves at the root node.
  MoveArray root_move_array_;

  std::vector<IterationStat> iteration_stats_;
};

template <Variant variant>
IDSResult IterativeDeepener<variant>::Search() {
  IDSResult ids_result;
  std::ostream& out = ids_params_.thinking_output ? std::cout : nullstream;
  StopWatch stop_watch;
  stop_watch.Start();

  root_move_array_ = GenerateMoves<variant>(board_);
  out << "# Number of moves at root: " << root_move_array_.size() << std::endl;

  // No moves to make. Just return by setting invalid move. This can happen if
  // Search() is called after game ends.
  if (root_move_array_.size() == 0) {
    ids_result.best_move = Move();
    ids_result.best_move_score = INF;
    return ids_result;
  }

  // Caller provided move ordering and pruning takes priority over move orderer.
  if (ids_params_.pruned_ordered_moves.size()) {
    root_move_array_ = ids_params_.pruned_ordered_moves;
  } else {
    const MoveInfoArray move_info_array = OrderMovesByEvalScore<variant>(
        board_, egtb_, root_move_array_, nullptr);
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
    ids_result.best_move = root_move_array_.get(0);
    ids_result.best_move_score = INF;
    return ids_result;
  }

  // Iterative deepening starts here.
  for (int depth = 1; depth <= ids_params_.search_depth; ++depth) {
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
    } else if (ids_params_.pruned_ordered_moves.size() == 0) {
      const std::optional<TTData> tdata = transpos_.Get(board_.ZobristKey());
      if (tdata && tdata->best_move.is_valid()) {
        root_move_array_.PushToFront(tdata->best_move);
      }
    }

    const auto istat = FindBestMove(depth);
    iteration_stats_.push_back(istat);

    double elapsed_time = stop_watch.ElapsedTime();

    const IterationStat& last_istat = iteration_stats_.back();

    // If FindMove could not complete at least the first root move subtree
    // completely, don't report stats or update best_move as the results are
    // likely to be incorrect.
    if (timer_.Lapsed() && last_istat.root_moves_covered == 0) {
      break;
    }

    // Update best_move and stats.
    ids_result.best_move = last_istat.best_move;
    ids_result.best_move_score = last_istat.score;
    for (const auto& stat : last_istat.move_stats) {
      ids_result.id_search_stats.nodes_searched += stat.second.nodes_searched;
      ids_result.id_search_stats.search_depth = stat.second.search_depth;
    }

    // Thinking output in XBoard or UCI format.
    if (ids_params_.thinking_output) {
      if (ids_params_.uci_output_format) {
        // UCI info format: info depth <d> score cp <score> time <time> nodes <nodes> pv <moves>
        std::cout << "info depth " << depth 
                  << " score cp " << last_istat.score
                  << " time " << int(elapsed_time * 10)  // Convert centiseconds to milliseconds
                  << " nodes " << ids_result.id_search_stats.nodes_searched
                  << " pv " << PV(ids_result.best_move) << std::endl;
      } else {
        // XBoard style thinking output.
        char output[256];
        snprintf(output, 256, "%2d\t%5d\t%5d\t%10lu\t%s", depth, last_istat.score,
                 int(elapsed_time), ids_result.id_search_stats.nodes_searched,
                 PV(ids_result.best_move).c_str());
        std::cout << output << std::endl;
      }
    }

    // Don't go any deeper if a win is confirmed or timer has lapsed.
    if (last_istat.score == WIN || timer_.Lapsed()) {
      break;
    }
  }
  stop_watch.Stop();
  out << "# Time taken for ID search: " << stop_watch.ElapsedTime() << " centis"
      << std::endl;
  return ids_result;
}

// Finds the best move by searching up to given max_depth. Stops and returns
// quickly if timer expires during computation. Updates iteration_stats_ with
// details of current iteration.
template <Variant variant>
IterationStat IterativeDeepener<variant>::FindBestMove(int max_depth) {

  auto search = [max_depth, root_move_array = root_move_array_,
                 &transpos = transpos_,
                 egtb = egtb_](int thread_num,
                               Board board /* copy of board for each thread */,
                               Timer& timer, IterationStat* ret_istat) mutable {
    if (thread_num % 2 == 1) {
      ++max_depth;
    }
    PVSearch<variant> pv_search(board, &timer, transpos, egtb);
    IterationStat istat;
    istat.depth = max_depth;
    istat.best_move = root_move_array.get(0);
    istat.score = -INF;
    istat.root_moves_covered = 0;
    for (unsigned int i = 0; i < root_move_array.size(); ++i) {
      const Move& move = root_move_array.get(i);
      board.MakeMove(move);
      SearchStats search_stats;
      int score = -INF;
      if (i == 0 || max_depth < 5) {
        score =
            -pv_search.Search(max_depth - 1, -INF, -istat.score, search_stats);
      } else {
        bool lmr_triggered = false;
        if (i >= 4 && max_depth >= 2) {
          score = -pv_search.Search(max_depth - 2, -istat.score - 1,
                                    -istat.score, search_stats);
          lmr_triggered = true;
        }
        if (!lmr_triggered || score > istat.score) {
          score = -pv_search.Search(max_depth - 1, -istat.score - 1,
                                    -istat.score, search_stats);
        }
        if (score > istat.score) {
          score = -pv_search.Search(max_depth - 1, -INF, -istat.score,
                                    search_stats);
        }
      }
      board.UnmakeLastMove();
      istat.move_stats.push_back(std::make_pair(move, search_stats));

      // Return on timer expiry only if we are not searching at depth 1. If
      // searching at depth 1, we should at least quickly find a meaningful
      // move even if timer expires before all the root moves are evaluated.
      // Without this, there is a possibility of iterative deepener not
      // reporting any moves at all in some extremely time constrained
      // situations. Searching all root moves at depth 1 is very quick
      // (sub-millisecond latency).
      if (timer.Lapsed() && max_depth > 1) {
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
      transpos.Put(istat.score, NodeType::EXACT_NODE, max_depth,
                   board.ZobristKey(), istat.best_move);
    }
    *ret_istat = istat;
  };

  const int num_threads = (max_depth < 3 ? 1 : NUM_THREADS);
  std::vector<std::thread> threads;
  std::vector<IterationStat> istats(num_threads);

  Timer threads_timer;
  threads_timer.Run();

  // Run num_threads - 1 search threads.
  for (int i = 1; i < num_threads; ++i) {
    threads.push_back(
        std::thread(search, i, board_, std::ref(threads_timer), &istats.at(i)));
  }

  // Run search in main thread.
  search(0, board_, timer_, &istats.at(0));
  threads_timer.Invalidate();

  for (auto& thread : threads) {
    thread.join();
  }

  for (size_t i = 1; i < istats.size(); ++i) {
    istats.at(0).MergeStats(istats.at(i));
  }
  return istats.at(0);
}

template <Variant variant>
std::string IterativeDeepener<variant>::PV(const Move& root_move) {
  std::string pv;

  // Root move may not always be in the transposition table or the one in
  // transposition table may not be the first move of PV if move ordering
  // is determined externally.
  pv.append(SAN(board_, root_move) + " ");
  board_.MakeMove(root_move);

  int depth = 0;
  while (depth < 10) {
    U64 zkey = board_.ZobristKey();
    const std::optional<TTData> tdata = transpos_.Get(zkey);
    if (!tdata || !tdata->best_move.is_valid() ||
        tdata->node_type() != NodeType::EXACT_NODE) {
      break;
    }
    pv.append(SAN(board_, tdata->best_move) + " ");
    ++depth;
    board_.MakeMove(tdata->best_move);
  }
  while (depth--) {
    board_.UnmakeLastMove();
  }

  board_.UnmakeLastMove(); // root move
  return pv;
}

} // namespace

template <Variant variant>
IDSResult IDSearch(const IDSParams& ids_params, Board& board, Timer& timer,
                   TranspositionTable& transpos, EGTB* egtb) {
  return IterativeDeepener<variant>(ids_params, board, timer, transpos, egtb)
      .Search();
}

template IDSResult IDSearch<Variant::STANDARD>(const IDSParams& ids_params,
                                               Board& board, Timer& timer,
                                               TranspositionTable& transpos,
                                               EGTB* egtb);

template IDSResult IDSearch<Variant::ANTICHESS>(const IDSParams& ids_params,
                                                Board& board, Timer& timer,
                                                TranspositionTable& transpos,
                                                EGTB* egtb);

template IDSResult IDSearch<Variant::SUICIDE>(const IDSParams& ids_params,
                                              Board& board, Timer& timer,
                                              TranspositionTable& transpos,
                                              EGTB* egtb);
