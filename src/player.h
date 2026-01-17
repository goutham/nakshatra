#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"
#include "common.h"
#include "egtb.h"
#include "move.h"
#include "timer.h"
#include "transpos.h"

#include <signal.h>
#include <sys/time.h>

struct SearchParams {
  bool thinking_output = false;
  int search_depth = MAX_DEPTH;
  bool antichess_pns = true;
};

class Player {
public:
  Player(const Variant variant, Board& board, TranspositionTable& transpos,
         Timer& timer)
      : variant_(variant), board_(board), transpos_(transpos), timer_(timer),
        egtb_(GetEGTB(variant)) {
    for (int i = 0; i < 12; ++i) {
      for (int j = 0; j < 64; ++j) {
        history_[i][j] = 0;
      }
    }
  }

  Move Search(const SearchParams& search_params, long time_for_move_centis);

private:
  template <Variant variant>
  Move SearchInternal(const SearchParams& search_params,
                      long time_for_move_centis);

  const Variant variant_;
  Board& board_;
  TranspositionTable& transpos_;
  Timer& timer_;
  EGTB* egtb_;
  int history_[12][64];
};

#endif
