#ifndef EVAL_NORMAL_H
#define EVAL_NORMAL_H

#include "common.h"
#include "eval.h"

class Board;

namespace movegen {
class MoveGenerator;
}

namespace eval {

// Evaluates the board to come up with a score.
class EvalNormal : public Evaluator {
 public:
  EvalNormal(Board* board,
             movegen::MoveGenerator* movegen)
      : board_(board),
        movegen_(movegen) {}

  int Evaluate() override;

  int Result() const override;

 private:
  int PieceValDifference() const;

  Board* board_;
  movegen::MoveGenerator* movegen_;
};

}  // namespace eval

#endif
