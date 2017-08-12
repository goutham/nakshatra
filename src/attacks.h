#ifndef ATTACKS_H
#define ATTACKS_H

#include "common.h"
#include "piece.h"

namespace attacks {

// Computes attack bitboard for given non-pawn piece from given index.
U64 Attacks(const U64 bitboard, const int index, const Piece piece);

} // namespace attacks

#endif
