#ifndef SIDE_RELATIVE_H
#define SIDE_RELATIVE_H

#include "common.h"

#include <stdexcept>

namespace side_relative {

// Shifts left relative to the side (so, for Side::BLACK, relative left shift is
// actual right shift).
template <Side side>
constexpr U64 LeftShift(U64 bitboard, int shift);

template <>
constexpr U64 LeftShift<Side::WHITE>(U64 bitboard, int shift) {
  return bitboard << shift;
}

template <>
constexpr U64 LeftShift<Side::BLACK>(U64 bitboard, int shift) {
  return bitboard >> shift;
}

template <Side side>
constexpr U64 RightShift(U64 bitboard, int shift);

template <>
constexpr U64 RightShift<Side::WHITE>(U64 bitboard, int shift) {
  return bitboard >> shift;
}

template <>
constexpr U64 RightShift<Side::BLACK>(U64 bitboard, int shift) {
  return bitboard << shift;
}

template <Side side>
constexpr U64 MaskRow(int row) {
  return row >= 0 && row <= 7
             ? (side == Side::WHITE ? (0xFFULL << (row * 8))
                                    : (0xFF00000000000000ULL >> (row * 8)))
             : throw std::logic_error("invalid row");
}

template <Side side>
constexpr U64 MaskColumn(int col) {
  return col >= 0 && col <= 7
             ? (side == Side::WHITE ? (0x8080808080808080ULL >> col)
                                    : (0x0101010101010101ULL << col))
             : throw std::logic_error("invalid col");
}

// Pushes bits one row to the front corresponding to the side.
template <Side side>
constexpr U64 PushFront(U64 bitboard) {
  return LeftShift<side>(bitboard, 8);
}

// Pushes bits to the north-east direction corresponding to the side.
template <Side side>
constexpr U64 PushNorthEast(U64 bitboard) {
  return LeftShift<side>(bitboard & ~MaskColumn<side>(7), 7);
}

// Pushes bits to the north-west direction corresponding to the side.
template <Side side>
constexpr U64 PushNorthWest(U64 bitboard) {
  return LeftShift<side>(bitboard & ~MaskColumn<side>(0), 9);
}

} // namespace side_relative

#endif
