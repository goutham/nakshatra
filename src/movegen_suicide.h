#ifndef MOVEGEN_SUICIDE_H
#define MOVEGEN_SUICIDE_H

#include "movegen.h"
#include "move.h"

class Board;

namespace movegen {

// Move generator for suicide chess.
class MoveGeneratorSuicide : public MoveGenerator {
 public:
  MoveGeneratorSuicide(const Board& board) : board_(board) {}

  void GenerateMoves(MoveArray* move_array) override;

  bool IsValidMove(const Move& move) override;

 private:
  const Board& board_;
};

}  // namespace movegen

#endif
