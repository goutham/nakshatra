#ifndef EVAL_SUICIDE_H
#define EVAL_SUICIDE_H

#include "common.h"
#include "eval.h"

class Board;
class EGTB;
class MoveArray;
class MoveGenerator;

class EvalSuicide : public Evaluator {
 public:
  EvalSuicide(Board* board,
              MoveGenerator* movegen,
              EGTB* egtb)
      : board_(board),
        movegen_(movegen),
        egtb_(egtb) {}

  int Evaluate() override;

  int Result() const override;

 private:
  int PieceValDifference() const;

  int OpponentMobility(const MoveArray& move_array);

  int InspectResultOfMove();

  bool RivalBishopsOnOppositeColoredSquares() const;

  Board* board_;
  MoveGenerator* movegen_;
  EGTB* egtb_;
};

#endif
