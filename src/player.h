#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"
#include "common.h"
#include "move.h"
#include "timer.h"

#include <signal.h>
#include <sys/time.h>

class EGTB;
class MoveGenerator;
class MoveOrderer;
class TranspositionTable;
class Evaluator;

struct SearchParams {
  bool thinking_output = false;
  int search_depth = MAX_DEPTH;
  bool antichess_pns = true;
};

class Player {
public:
  Player(const Variant variant, Board* board, MoveGenerator* movegen,
         TranspositionTable* transpos, Evaluator* evaluator, Timer* timer,
         EGTB* egtb)
      : variant_(variant), board_(board), movegen_(movegen),
        transpos_(transpos), evaluator_(evaluator), timer_(timer), egtb_(egtb) {
  }

  Move Search(const SearchParams& search_params, long time_for_move_centis);

private:
  const Variant variant_;
  Board* board_;
  MoveGenerator* movegen_;
  TranspositionTable* transpos_;
  Evaluator* evaluator_;
  Timer* timer_;
  EGTB* egtb_;
};

#endif
