#ifndef SIDE_RELATIVE_H
#define SIDE_RELATIVE_H

#include "common.h"

#include <cassert>
#include <stdexcept>

namespace bitmanip { // namespace for bit manipulation functions

constexpr U64 MaskRow(int row) {
  assert(row >= 0 && row <= 7);
  return 0xFFULL << (row * 8);
}

constexpr U64 MaskColumn(int col) {
  assert(col >= 0 && col <= 7);
  return 0x0101010101010101ULL << col;
}

constexpr U64 NOT_A_FILE = ~MaskColumn(0);
constexpr U64 NOT_H_FILE = ~MaskColumn(7);

constexpr U64 PushNorth(U64 bitboard) { return bitboard << 8; }

constexpr U64 PushSouth(U64 bitboard) { return bitboard >> 8; }

constexpr U64 PushEast(U64 bitboard) { return (bitboard & NOT_H_FILE) << 1; }

constexpr U64 PushWest(U64 bitboard) { return (bitboard & NOT_A_FILE) >> 1; }

constexpr U64 PushNorthEast(U64 bitboard) {
  return (bitboard & NOT_H_FILE) << 9;
}

constexpr U64 PushNorthWest(U64 bitboard) {
  return (bitboard & NOT_A_FILE) << 7;
}

constexpr U64 PushSouthEast(U64 bitboard) {
  return (bitboard & NOT_H_FILE) >> 7;
}

constexpr U64 PushSouthWest(U64 bitboard) {
  return (bitboard & NOT_A_FILE) >> 9;
}

inline U64 FromLayout(const char* layout_str) {
  // Layout indices:
  //     8| 56, 57, 58, 59, 60, 61, 62, 63,
  //     7| 48, 49, 50, 51, 52, 53, 54, 55,
  //     6| 40, 41, 42, 43, 44, 45, 46, 47,
  //     5| 32, 33, 34, 35, 36, 37, 38, 39,
  //     4| 24, 25, 26, 27, 28, 29, 30, 31,
  //     3| 16, 17, 18, 19, 20, 21, 22, 23,
  //     2|  8,  9, 10, 11, 12, 13, 14, 15,
  //     1|  0,  1,  2,  3,  4,  5,  6,  7,
  //        --  --  --  --  --  --  --  --
  //         A   B   C   D   E   F   G   H
  U64 bitboard = 0ULL;
  int sq = 0;
  for (int i = 0; layout_str[i] != '\0'; ++i) {
    if (layout_str[i] != '.' && layout_str[i] != '1') {
      continue;
    }
    const int index = (7 - (sq / 8)) * 8 + (sq % 8);
    sq++;
    if (sq > 64) {
      throw std::invalid_argument("Too many squares");
    }
    if (layout_str[i] == '.') {
      continue;
    }
    bitboard |= (1ULL << index);
  }
  if (sq < 64) {
    throw std::invalid_argument("Too few squares");
  }
  return bitboard;
}

namespace siderel { // namespace for side relative operations

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 MaskRow(int row) {
  return bitmanip::MaskRow(side == Side::WHITE ? row : 7 - row);
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 MaskColumn(int col) {
  return bitmanip::MaskColumn(side == Side::WHITE ? col : 7 - col);
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 PushNorth(U64 bitboard) {
  return side == Side::WHITE ? bitmanip::PushNorth(bitboard)
                             : bitmanip::PushSouth(bitboard);
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 PushNorthEast(U64 bitboard) {
  return side == Side::WHITE ? bitmanip::PushNorthEast(bitboard)
                             : bitmanip::PushSouthWest(bitboard);
}

template <Side side>
  requires(side == Side::WHITE || side == Side::BLACK)
U64 PushNorthWest(U64 bitboard) {
  return side == Side::WHITE ? bitmanip::PushNorthWest(bitboard)
                             : bitmanip::PushSouthEast(bitboard);
}

template <Side side>
U64 FrontFill(U64 bitboard) {
  if constexpr (side == Side::WHITE) {
    bitboard |= (bitboard << 8);
    bitboard |= (bitboard << 16);
    bitboard |= (bitboard << 32);
    return bitboard;
  } else {
    static_assert(side == Side::BLACK);
    bitboard |= (bitboard >> 8);
    bitboard |= (bitboard >> 16);
    bitboard |= (bitboard >> 32);
    return bitboard;
  }
}

template <Side side>
U64 RearFill(U64 bitboard) {
  return FrontFill<OppositeSide(side)>(bitboard);
}

template <Side side>
U64 FrontSpan(U64 bitboard) {
  return PushNorth<side>(FrontFill<side>(bitboard));
}

template <Side side>
U64 RearSpan(U64 bitboard) {
  return PushNorth<OppositeSide(side)>(RearFill<side>(bitboard));
}

} // namespace siderel
} // namespace bitmanip

#endif
