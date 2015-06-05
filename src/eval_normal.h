#ifndef EVAL_NORMAL_H
#define EVAL_NORMAL_H

#include "common.h"
#include "eval.h"

class Board;
class MoveGenerator;

// Evaluates the board to come up with a score.
class EvalNormal : public Evaluator {
 public:
  EvalNormal(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen) {}

  int Evaluate() override;

  int Result() const override;

 private:
  int PieceValDifference() const;

  Board* board_;
  MoveGenerator* movegen_;
};

#endif
