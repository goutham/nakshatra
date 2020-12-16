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
  EvalAntichess(Board* board, MoveGenerator* movegen, EGTB* egtb)
      : board_(board), movegen_(movegen), egtb_(egtb) {}

  int Evaluate() override;

  int Result() const override;

private:
  int PieceValDifference() const;

  int PieceCountDiff() const;

  int OpponentMobility(const MoveArray& move_array);

  int InspectResultOfMove();

  bool RivalBishopsOnOppositeColoredSquares() const;

  Board* board_;
  MoveGenerator* movegen_;
  EGTB* egtb_;
};

#endif
