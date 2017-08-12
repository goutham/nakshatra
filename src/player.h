#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"
#include "book.h"
#include "common.h"
#include "move.h"
#include "timer.h"

#include <signal.h>
#include <sys/time.h>

class EGTB;
class IterativeDeepener;
class MoveGenerator;
struct Extensions;

struct SearchParams {
  bool thinking_output = false;
  int search_depth = MAX_DEPTH;
};

class Player {
public:
  Player(const Book* book, Board* board, MoveGenerator* movegen,
         IterativeDeepener* iterative_deepener, Timer* timer, EGTB* egtb,
         Extensions* extensions);

  Move Search(const SearchParams& search_params, long time_for_move_centis);

  Board* GetBoard() { return board_; }

private:
  const Book* book_;
  Board* board_;
  MoveGenerator* movegen_;
  IterativeDeepener* iterative_deepener_;
  Timer* timer_;
  EGTB* egtb_;
  Extensions* extensions_;
};

#endif
