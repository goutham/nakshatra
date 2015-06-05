#ifndef MOVEGEN_NORMAL_H
#define MOVEGEN_NORMAL_H

#include "movegen.h"
#include "move.h"

class Board;

// Move generator for normal chess.
class MoveGeneratorNormal : public MoveGenerator {
 public:
  MoveGeneratorNormal(Board* board) : board_(board) {}

  void GenerateMoves(MoveArray* move_array) override;

  bool IsValidMove(const Move& move) override;

 private:
  Board* board_;
};

#endif
