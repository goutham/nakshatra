#ifndef MOVE_ORDER_H
#define MOVE_ORDER_H

#include "common.h"
#include "move.h"
#include "move_array.h"

#include <vector>

class Board;
class MoveGenerator;
class Evaluator;

// Container for any preferred moves supplied by caller that need to be treated
// with special care.
struct PrefMoves {
  Move tt_move = Move();
  Move killer1 = Move();
  Move killer2 = Move();
};

class MoveOrderer {
public:
  virtual ~MoveOrderer() {}

  virtual void Order(MoveArray* move_array,
                     const PrefMoves* pref_moves = nullptr) = 0;

protected:
  MoveOrderer() {}
};

class AntichessMoveOrderer : public MoveOrderer {
public:
  AntichessMoveOrderer(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen) {}
  ~AntichessMoveOrderer() override {}

  void Order(MoveArray* move_array,
             const PrefMoves* pref_moves = nullptr) override;

private:
  Board* board_;
  MoveGenerator* movegen_;
};

class StandardMoveOrderer : public MoveOrderer {
public:
  StandardMoveOrderer(Board* board) : board_(board) {}
  ~StandardMoveOrderer() override {}

  void Order(MoveArray* move_array,
             const PrefMoves* pref_moves = nullptr) override;

private:
  Board* board_;
};

class EvalScoreOrderer : public MoveOrderer {
public:
  EvalScoreOrderer(Evaluator* eval) : eval_(eval) {}
  ~EvalScoreOrderer() override {}

  void Order(MoveArray* move_array,
             const PrefMoves* pref_moves = nullptr) override;

private:
  Evaluator* eval_;
};

#endif
