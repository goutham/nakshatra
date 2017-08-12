#ifndef MOVE_ORDER_H
#define MOVE_ORDER_H

#include "common.h"
#include "move.h"
#include "move_array.h"

#include <vector>

class Board;
class MoveGenerator;

class MoveOrderer {
public:
  virtual ~MoveOrderer() {}

  virtual void Order(MoveArray* move_array) = 0;

protected:
  MoveOrderer() {}
};

class MobilityOrderer : public MoveOrderer {
public:
  MobilityOrderer(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen) {}
  ~MobilityOrderer() override {}

  void Order(MoveArray* move_array) override;

private:
  Board* board_;
  MoveGenerator* movegen_;
};

class CapturesFirstOrderer : public MoveOrderer {
public:
  CapturesFirstOrderer(Board* board) : board_(board) {}
  ~CapturesFirstOrderer() override {}

  void Order(MoveArray* move_array) override;

private:
  Board* board_;
};

#endif
