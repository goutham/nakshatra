#ifndef EVAL_STANDARD_H
#define EVAL_STANDARD_H

#include "common.h"
#include "eval.h"
#include "move_order.h"

class Board;
class MoveGenerator;

// Evaluates the board to come up with a score.
class EvalStandard : public Evaluator {
public:
  EvalStandard(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen), orderer_(board) {}

  int Evaluate(int alpha, int beta) override;

  int Result() const override;

private:
  int StaticEval() const;
  int Quiesce(int alpha, int beta);
  int PieceValDifference() const;

  Board* board_;
  MoveGenerator* movegen_;
  StandardMoveOrderer orderer_;
};

#endif
