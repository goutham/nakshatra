#include "pawns.h"
#include "bitmanip.h"
#include "board.h"
#include "common.h"
#include "movegen.h"

#include <stdexcept>

namespace pawns {

U64 DoubledPawns(const Board& board, const Side side) {
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  if (side == Side::BLACK) {
    return pawn_bitboard &
           bitmanip::siderel::RearSpan<Side::BLACK>(pawn_bitboard);
  } else if (side == Side::WHITE) {
    return pawn_bitboard &
           bitmanip::siderel::RearSpan<Side::WHITE>(pawn_bitboard);
  }
  throw std::logic_error("Invalid side passed to DoubledPawns.");
}

} // namespace pawns
