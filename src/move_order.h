#ifndef MOVE_ORDER_H
#define MOVE_ORDER_H

#include "common.h"
#include "move.h"
#include "move_array.h"

#include <algorithm>
#include <vector>

class Board;
class MoveGenerator;
class Evaluator;

enum class MoveType {
  TT,
  SEE_GOOD_CAPTURE,
  KILLER,
  QUIET,
  SEE_BAD_CAPTURE,
  UNCATEGORIZED
};

struct MoveInfo {
  Move move;
  MoveType type;
  int score;
};

struct MoveInfoArray {
  MoveInfo moves[256];
  size_t size;

  void Sort() {
    std::sort(
        moves, moves + size, [](const MoveInfo& a, const MoveInfo& b) -> bool {
          return a.type < b.type || (a.type == b.type && a.score > b.score);
        });
  }
};

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

  virtual void Order(const MoveArray& move_array, const PrefMoves* pref_moves,
                     MoveInfoArray* move_info_array) = 0;

protected:
  MoveOrderer() {}
};

class AntichessMoveOrderer : public MoveOrderer {
public:
  AntichessMoveOrderer(Board* board, MoveGenerator* movegen)
      : board_(board), movegen_(movegen) {}
  ~AntichessMoveOrderer() override {}

  void Order(const MoveArray& move_array, const PrefMoves* pref_moves,
             MoveInfoArray* move_info_array) override;

private:
  Board* board_;
  MoveGenerator* movegen_;
};

class StandardMoveOrderer : public MoveOrderer {
public:
  StandardMoveOrderer(Board* board) : board_(board) {}
  ~StandardMoveOrderer() override {}

  void Order(const MoveArray& move_array, const PrefMoves* pref_moves,
             MoveInfoArray* move_info_array) override;

private:
  Board* board_;
};

class EvalScoreOrderer : public MoveOrderer {
public:
  EvalScoreOrderer(Board* board, Evaluator* eval)
      : board_(board), eval_(eval) {}
  ~EvalScoreOrderer() override {}

  void Order(const MoveArray& move_array, const PrefMoves* pref_moves,
             MoveInfoArray* move_info_array) override;

private:
  Board* board_;
  Evaluator* eval_;
};

#endif
