#ifndef PV_SEARCH_H
#define PV_SEARCH_H

#include "board.h"
#include "egtb.h"
#include "move.h"
#include "stats.h"
#include "timer.h"
#include "transpos.h"

template <Variant variant>
class PVSearch {
public:
  PVSearch(Board& board, Timer* timer, TranspositionTable& transpos, EGTB* egtb)
      : board_(board), timer_(timer), transpos_(transpos), egtb_(egtb) {}

  int Search(int max_depth, int alpha, int beta, SearchStats& search_stats);

private:
  int PVS(int max_depth, int alpha, int beta, int ply, bool allow_null_move,
          SearchStats& search_stats);

  Board& board_;
  Timer* timer_;
  TranspositionTable& transpos_;
  EGTB* egtb_;
  Move killers_[MAX_DEPTH][2];
};

#endif
