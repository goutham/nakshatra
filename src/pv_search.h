#ifndef PV_SEARCH_H
#define PV_SEARCH_H

#include "board.h"
#include "egtb.h"
#include "history.h"
#include "move.h"
#include "stats.h"
#include "timer.h"
#include "transpos.h"

template <Variant variant>
class PVSearch {
public:
  PVSearch(Board& board, Timer* timer, TranspositionTable& transpos, EGTB* egtb, HistoryTable& history)
      : board_(board), timer_(timer), transpos_(transpos), egtb_(egtb), history_(history) {}

  int Search(int max_depth, int alpha, int beta, SearchStats& search_stats);

private:
  int PVS(int max_depth, int alpha, int beta, int ply, bool allow_null_move,
          SearchStats& search_stats);

  Board& board_;
  Timer* timer_;
  TranspositionTable& transpos_;
  EGTB* egtb_;
  Move killers_[MAX_DEPTH][2];
  HistoryTable& history_;

  void UpdateHistory(const Move& move, int depth, bool is_bonus);
};

#endif
