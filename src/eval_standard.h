#ifndef EVAL_STANDARD_H
#define EVAL_STANDARD_H

#include "common.h"
#include "eval.h"

class Board;
class MoveGenerator;

// Evaluates the board to come up with a score.
class EvalStandard : public Evaluator {
public:
  EvalStandard(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen) {}

  int Evaluate() override;

  int Result() const override;

private:
  int PieceValDifference() const;

  Board* board_;
  MoveGenerator* movegen_;
};

#endif
