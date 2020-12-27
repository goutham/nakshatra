#ifndef ATTACKS_H
#define ATTACKS_H

#include "board.h"
#include "common.h"
#include "piece.h"

namespace attacks {

// Computes attack bitboard for given non-pawn piece from given index.
U64 Attacks(const U64 bitboard, const int index, const Piece piece);

// Returns bitboard of the attackers of the given square. The side of attacking
// piece is used for direction of pawn moves (so PAWN != -PAWN). Enpassants are
// ignored.
U64 SquareAttackers(const int square, const Piece attacking_piece,
                    const U64 occ, const U64 attacking_side_piece_occ);

// Returns true if king of given side is in check.
bool IsKingInCheck(const Board& board, const Side side);

} // namespace attacks

#endif
