#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"
#include "common.h"
#include "move.h"
#include "timer.h"

#include <signal.h>
#include <sys/time.h>

class EGTB;
class IterativeDeepener;
class MoveGenerator;
class TranspositionTable;
struct Extensions;

struct SearchParams {
  bool thinking_output = false;
  int search_depth = MAX_DEPTH;
};

class Player {
public:
  Player(Board* board, MoveGenerator* movegen,
         IterativeDeepener* iterative_deepener, TranspositionTable* transpos,
         Timer* timer, EGTB* egtb, Extensions* extensions, int rand_moves);

  Move Search(const SearchParams& search_params, long time_for_move_centis);

  Board* GetBoard() { return board_; }

private:
  Board* board_;
  MoveGenerator* movegen_;
  IterativeDeepener* iterative_deepener_;
  TranspositionTable* transpos_;
  Timer* timer_;
  EGTB* egtb_;
  Extensions* extensions_;
  int rand_moves_ = 0;
};

#endif
