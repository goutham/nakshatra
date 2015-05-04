#ifndef SAN_H
#define SAN_H

#include <string>

#include "board.h"
#include "move.h"

namespace {
class MoveGenerator;
}

// Return the standard algebraic notation (SAN) of the move.
std::string SAN(const Board& board, const Move& move);

Move SANToMove(const std::string& move_san,
               const Board& board,
               movegen::MoveGenerator* movegen);

#endif
