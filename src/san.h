#ifndef SAN_H
#define SAN_H

#include <string>

#include "board.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"

// Return the standard algebraic notation (SAN) of the move.
std::string SAN(const Board& board, const Move& move);

template <Variant variant>
Move SANToMove(const std::string& move_san, Board* board) {
  MoveArray move_array;
  GenerateMoves<variant>(board, &move_array);
  for (size_t i = 0; i < move_array.size(); ++i) {
    if (const Move& move = move_array.get(i); SAN(*board, move) == move_san) {
      return move;
    }
  }
  throw std::runtime_error("Unknown move " + move_san);
}

#endif
