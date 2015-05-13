#include "board.h"
#include "book.h"
#include "common.h"
#include "egtb.h"
#include "extensions.h"
#include "iterative_deepener.h"
#include "movegen.h"
#include "piece.h"
#include "player.h"
#include "pn_search.h"
#include "stopwatch.h"
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

namespace search {

namespace {
// Define a line with 2 points - (x1, y1) and (x2, y2). Returns the
// y-coordinate of a third point whose x-coordinate is given - x3.
inline double PointOnLine(double x1,
                          double y1,
                          double x2,
                          double y2,
                          double x3) {
  return y1 + ((y2 - y1) / (x2 - x1)) * (x3 - x1);
}

long AllocateTime(long time_centis, long otime_centis) {
  if (time_centis >= 60000) {
    return 2250l;
  }
  if (time_centis >= 6000) {
    return PointOnLine(6000, 500, 60000, 2250, time_centis);
  }
  if (time_centis >= 1000) {
    return PointOnLine(1000, 200, 6000, 500, time_centis);
  }
  if (time_centis >= 200) {
    return PointOnLine(200, 20, 1000, 200, time_centis);
  }
  return 10l;
}
}

Player::Player(const Book* book,
               Board* board,
               IterativeDeepener* iterative_deepener,
               Timer* timer,
               EGTB* egtb,
               Extensions* extensions)
  : book_(book),
    board_(board),
    iterative_deepener_(iterative_deepener),
    timer_(timer),
    egtb_(egtb),
    extensions_(extensions) {}

Move Player::Search(const SearchParams& search_params,
                    long time_centis,
                    long otime_centis) {
  if (book_) {
    Move book_move = book_->GetBookMove(*board_);
    if (book_move.is_valid()) {
      return book_move;
    }
  }
  if (egtb_ &&
      OnlyOneBitSet(board_->BitBoard(Side::WHITE)) &&
      OnlyOneBitSet(board_->BitBoard(Side::BLACK))) {
    std::cout << "# Num pieces <= 2, looking up EGTB..." << std::endl;
    const EGTBIndexEntry* egtb_entry = egtb_->Lookup();
    if (egtb_entry) {
      PrintEGTBIndexEntry(*egtb_entry);
      return egtb_entry->next_move;
    } else {
      std::cout << "# Num pieces <= 2, but EGTB missed!" << std::endl;
    }
  }

  // Time allocated to search for the best move.
  long time_for_move_centis = AllocateTime(time_centis, otime_centis);

  IDSParams ids_params;
  ids_params.thinking_output = search_params.thinking_output;

  if (extensions_->pn_search &&
      time_for_move_centis > 50 &&
      movegen::CountMoves(board_->SideToMove(), *board_) > 1) {
    assert (extensions_->pns_timer);

    // Allocate 15% of total time for PNS.
    extensions_->pns_timer->Reset();
    extensions_->pns_timer->Run(time_for_move_centis * 0.05);

    StopWatch pn_stop_watch;
    pn_stop_watch.Start();

    PNSParams pns_params;
    PNSResult pns_result;
    extensions_->pn_search->Search(pns_params, &pns_result);
    pn_stop_watch.Stop();
    std::cout << "# PNS time: " << pn_stop_watch.ElapsedTime() << " centis"
              << std::endl;
    std::cout << "# PNS Stats: num_nodes: " << pns_result.num_nodes
              << std::endl;

    PNSResult::MoveStat best_pns_move_stat = pns_result.ordered_moves.at(0);
    // If the best move is WON, return.
    if (best_pns_move_stat.result == WIN) {
      std::cout << "# PNS WIN!" << std::endl;
      return best_pns_move_stat.move;
    }
    // If the best move is lost, let Negascout search find the best move.
    if (best_pns_move_stat.result == -WIN) {
      std::cout << "# PNS LOSS!" << std::endl;
    } else {
      // Include all non-LOSS moves.
      for (const PNSResult::MoveStat& move_stat : pns_result.ordered_moves) {
        if (move_stat.result == -WIN) break;
        ids_params.pruned_ordered_moves.Add(move_stat.move);
      }
    }

    // Subtract time taken by PNSearch.
    time_for_move_centis -= static_cast<long>(pn_stop_watch.ElapsedTime());
  }

  timer_->Reset();
  timer_->Run(time_for_move_centis);

  SearchStats id_search_stats;
  int move_score;
  Move best_move;
  iterative_deepener_->Search(ids_params,
                              &best_move,
                              &move_score,
                              &id_search_stats);
  return best_move;
}

}  // namespace search
