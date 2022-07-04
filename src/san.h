#ifndef SAN_H
#define SAN_H

#include <string>

#include "board.h"
#include "move.h"

// Return the standard algebraic notation (SAN) of the move.
std::string SAN(const Board& board, const Move& move);

Move SANToMove(const Variant variant, const std::string& move_san,
               Board* board);

#endif
