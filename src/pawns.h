#ifndef PAWNS_H
#define PAWNS_H

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

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 IsolatedPawns(const Board& board) {
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  const U64 front_fill = bitmanip::siderel::FrontFill<side>(pawn_bitboard);
  const U64 rear_fill = bitmanip::siderel::RearFill<side>(pawn_bitboard);
  const U64 file_fill = front_fill | rear_fill;
  const U64 neighbor_files_fill =
      bitmanip::PushEast(file_fill) | bitmanip::PushWest(file_fill);
  return pawn_bitboard & ~neighbor_files_fill;
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 DefendedPawns(const Board& board) {
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  const U64 se_defenders = bitmanip::siderel::PushSouthEast<side>(pawn_bitboard) & pawn_bitboard;
  const U64 sw_defenders = bitmanip::siderel::PushSouthWest<side>(pawn_bitboard) & pawn_bitboard;
  const U64 se_nw = bitmanip::siderel::PushNorthWest<side>(se_defenders);
  const U64 sw_ne = bitmanip::siderel::PushNorthEast<side>(sw_defenders);
  return se_nw | sw_ne;
}

} // namespace pawns

#endif