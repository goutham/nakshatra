#include "board.h"
#include "common.h"
#include "movegen.h"
#include "pawns.h"

#include <stdexcept>
namespace pawns {

namespace {

// Shifts the bitboard one row to the front corresponding to the side.
template <Side side> U64 ShiftFront(U64 bitboard);
template <> U64 ShiftFront<Side::WHITE>(U64 bitboard) { return bitboard << 8; }
template <> U64 ShiftFront<Side::BLACK>(U64 bitboard) { return bitboard >> 8; }

template <Side side>
U64 FrontFill(U64 bitboard);

template <>
U64 FrontFill<Side::WHITE>(U64 bitboard) {
  bitboard |= (bitboard << 8);
  bitboard |= (bitboard << 16);
  bitboard |= (bitboard << 32);
  return bitboard;
}

template <>
U64 FrontFill<Side::BLACK>(U64 bitboard) {
  bitboard |= (bitboard >> 8);
  bitboard |= (bitboard >> 16);
  bitboard |= (bitboard >> 32);
  return bitboard;
}

template <Side side>
U64 RearFill(U64 bitboard) {
  return FrontFill<OppositeSide(side)>(bitboard);
}

template <Side side>
U64 FrontSpan(U64 bitboard) {
  return ShiftFront<side>(FrontFill<side>(bitboard));
}

template <Side side>
U64 RearSpan(U64 bitboard) {
  return ShiftFront<OppositeSide(side)>(RearFill<side>(bitboard));
}

}  // namespace

U64 DoubledPawns(const Board& board, const Side side) {
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  return pawn_bitboard &
         (side == Side::WHITE
              ? RearSpan<Side::WHITE>(pawn_bitboard)
              : (side == Side::BLACK ? RearSpan<Side::BLACK>(pawn_bitboard)
                                     : throw std::logic_error("Invalid side")));
}

}  // namespace pawns
