#ifndef EVAL_ANTICHESS_H
#define EVAL_ANTICHESS_H

#include "common.h"
#include "eval.h"

class Board;
class EGTB;
class MoveArray;
class MoveGenerator;

class EvalAntichess : public Evaluator {
public:
  EvalAntichess(Board* board) : board_(board) {}

  int Evaluate(int alpha, int beta) override;

  int Result() const override;

private:
  int EvaluateInternal(int alpha, int beta, int max_depth);

  int OpponentMobility(const MoveArray& move_array);

  int InspectResultOfMove();

  bool RivalBishopsOnOppositeColoredSquares() const;

  Board* board_;
};

#endif
