#ifndef MOVE_ORDER_H
#define MOVE_ORDER_H

#include "common.h"
#include "move.h"
#include "move_array.h"

#include <vector>

class Board;

namespace search {

class MoveOrderer {
 public:
  virtual ~MoveOrderer() {}

  virtual void Order(MoveArray* move_array) = 0;

 protected:
  MoveOrderer() {}
};

class MobilityOrderer : public MoveOrderer {
 public:
  MobilityOrderer(Board* board)
      : board_(board) {}
  ~MobilityOrderer() override {}

  void Order(MoveArray* move_array) override;

 private:
  Board* board_;
};

class CapturesFirstOrderer : public MoveOrderer {
 public:
  CapturesFirstOrderer(Board* board)
      : board_(board) {}
  ~CapturesFirstOrderer() override {}

  void Order(MoveArray* move_array) override;

 private:
  Board* board_;
};

}  // namespace search

#endif
