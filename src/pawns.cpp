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

U64 PassedPawns(const Board& board, const Side side) {
  if (side == Side::WHITE) {
    U64 b_front_span =
        bitmanip::siderel::FrontSpan<Side::BLACK>(board.BitBoard(-PAWN));
    b_front_span |=
        bitmanip::PushEast(b_front_span) | bitmanip::PushWest(b_front_span);
    return board.BitBoard(PAWN) & ~b_front_span;
  } else if (side == Side::BLACK) {
    U64 w_front_span =
        bitmanip::siderel::FrontSpan<Side::WHITE>(board.BitBoard(PAWN));
    w_front_span |=
        bitmanip::PushEast(w_front_span) | bitmanip::PushWest(w_front_span);
    return board.BitBoard(-PAWN) & ~w_front_span;
  }
  throw std::invalid_argument("Invalid side passed to PassedPawns.");
}

} // namespace pawns
