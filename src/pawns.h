#include "bitmanip.h"
#include "board.h"
#include "common.h"

namespace pawns {

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 DoubledPawns(const Board& board) {
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  return pawn_bitboard & bitmanip::siderel::RearSpan<side>(pawn_bitboard);
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 PassedPawns(const Board& board) {
  constexpr Side opp_side = OppositeSide(side);
  U64 front_span = bitmanip::siderel::FrontSpan<opp_side>(
      board.BitBoard(PieceOfSide(PAWN, opp_side)));
  front_span |= bitmanip::PushEast(front_span) | bitmanip::PushWest(front_span);
  return board.BitBoard(PieceOfSide(PAWN, side)) & ~front_span;
}

} // namespace pawns
