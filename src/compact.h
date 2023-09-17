#ifndef __COMPACT_H__
#define __COMPACT_H__

#include <cstring>

#include "common.h"

struct BoardDesc {
  U64 bitboard_pieces[12];
  Side side_to_move;
  unsigned char castle;
  int ep_index;

  bool operator<(const BoardDesc& other) const {
    for (size_t i = 0; i < 12; ++i) {
      if (bitboard_pieces[i] < other.bitboard_pieces[i])
        return true;
      if (bitboard_pieces[i] > other.bitboard_pieces[i])
        return false;
    }
    if (side_to_move < other.side_to_move)
      return true;
    if (side_to_move > other.side_to_move)
      return false;
    if (castle < other.castle)
      return true;
    if (castle > other.castle)
      return false;

    return ep_index < other.ep_index;
  }
};

inline BoardDesc MakeCompactBoardDesc(const U64 bitboard_pieces[],
                                      Side side_to_move, unsigned char castle,
                                      int ep_index) {
  BoardDesc board_desc;
  std::memcpy(board_desc.bitboard_pieces, bitboard_pieces, sizeof(U64) * 12);
  board_desc.side_to_move = side_to_move;
  board_desc.castle = castle;
  board_desc.ep_index = ep_index;
  return board_desc;
}

#endif