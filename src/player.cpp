#include "player.h"
#include "board.h"
#include "book.h"
#include "common.h"
#include "egtb.h"
#include "extensions.h"
#include "iterative_deepener.h"
#include "movegen.h"
#include "piece.h"
#include "pn_search.h"
#include "stopwatch.h"
#include "timer.h"
#include "transpos.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <signal.h>
#include <sys/time.h>

Player::Player(const Book* book, Board* board, MoveGenerator* movegen,
               IterativeDeepener* iterative_deepener,
               TranspositionTable* transpos, Timer* timer, EGTB* egtb,
               Extensions* extensions, int rand_moves)
    : book_(book), board_(board), movegen_(movegen),
      iterative_deepener_(iterative_deepener), transpos_(transpos),
      timer_(timer), egtb_(egtb), extensions_(extensions),
      rand_moves_(rand_moves) {}

Move Player::Search(const SearchParams& search_params,
                    long time_for_move_centis) {
  transpos_->SetEpoch(board_->HalfMoves());

  std::ostream& out = search_params.thinking_output ? std::cout : nullstream;
  if (rand_moves_ > 0) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    --rand_moves_;
    return move_array.get(rand() % move_array.size());
  }
  if (book_) {
    Move book_move = book_->GetBookMove(*board_);
    if (book_move.is_valid()) {
      return book_move;
    }
  }
  if (egtb_ && OnlyOneBitSet(board_->BitBoard(Side::WHITE)) &&
      OnlyOneBitSet(board_->BitBoard(Side::BLACK))) {
    out << "# Num pieces <= 2, looking up EGTB..." << std::endl;
    const EGTBIndexEntry* egtb_entry = egtb_->Lookup();
    if (egtb_entry) {
      PrintEGTBIndexEntry(*egtb_entry);
      return egtb_entry->next_move;
    } else {
      out << "# Num pieces <= 2, but EGTB missed!" << std::endl;
    }
  }
  // If the move is forced, just move.
  if (movegen_->CountMoves() == 1) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    return move_array.get(0);
  }

  IDSParams ids_params;
  ids_params.thinking_output = search_params.thinking_output;
  ids_params.search_depth = search_params.search_depth;

  if (const auto& pns_extension = extensions_->pns_extension;
      pns_extension.pn_search) {
    assert(pns_extension.pns_timer);

    pns_extension.pns_timer->Reset();
    pns_extension.pns_timer->Run(
        time_for_move_centis *
        (pns_extension.pns_time_for_move_percent / 100.0));

    StopWatch pn_stop_watch;
    pn_stop_watch.Start();

    PNSParams pns_params;
    pns_params.quiet = !search_params.thinking_output;
    pns_params.max_nodes = 10000000;
    PNSResult pns_result;
    pns_extension.pn_search->Search(pns_params, &pns_result);
    pn_stop_watch.Stop();
    out << "# PNS time: " << pn_stop_watch.ElapsedTime() << " centis"
        << std::endl;

    if (pns_result.ordered_moves.size()) {
      out << "# PNS Stats: tree_size: " << pns_result.tree_size << std::endl;
      PNSResult::MoveStat best_pns_move_stat = pns_result.ordered_moves.at(0);
      out << "# PNS best move: " << best_pns_move_stat.move.str() << std::endl;
      // If 100% time is used for PNS, return best move.
      if (pns_extension.pns_time_for_move_percent >= 100) {
        return best_pns_move_stat.move;
      }
      // If the best move is WON, return.
      if (best_pns_move_stat.result == WIN) {
        out << "# PNS WIN!" << std::endl;
        return best_pns_move_stat.move;
      }
      // If the best move is lost, let Negascout search find the best move.
      if (best_pns_move_stat.result == -WIN) {
        out << "# PNS LOSS!" << std::endl;
      } else {
        // Include all non-LOSS moves.
        for (const PNSResult::MoveStat& move_stat : pns_result.ordered_moves) {
          if (move_stat.result == -WIN)
            break;
          ids_params.pruned_ordered_moves.Add(move_stat.move);
        }
      }
    } else {
      out << "# No PNS moves.";
    }

    // Subtract time taken by PNSearch.
    time_for_move_centis -= static_cast<long>(pn_stop_watch.ElapsedTime());
    out << "# Time left: " << time_for_move_centis << " centis" << std::endl;
  }

  timer_->Reset();
  timer_->Run(time_for_move_centis);

  SearchStats id_search_stats;
  int move_score;
  Move best_move;
  iterative_deepener_->Search(ids_params, &best_move, &move_score,
                              &id_search_stats);
  return best_move;
}
