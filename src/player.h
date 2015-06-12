#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"
#include "book.h"
#include "move.h"
#include "timer.h"

#include <signal.h>
#include <sys/time.h>

class EGTB;
class IterativeDeepener;
class MoveGenerator;
struct Extensions;

struct SearchParams {
  SearchParams() : thinking_output(false) {}
  bool thinking_output;
};

class Player {
 public:
  Player(const Book* book,
         Board* board,
         MoveGenerator* movegen,
         IterativeDeepener* iterative_deepener,
         Timer* timer,
         EGTB* egtb,
         Extensions* extensions);

  Move Search(const SearchParams& search_params,
              long time_centis,
              long otime_centis);

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
