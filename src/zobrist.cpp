#include "common.h"
#include "piece.h"
#include "zobrist.h"

#include <iostream>
#include <cstdlib>

namespace {

bool initialized = false;

U64 zobrist_[PIECE_MAX][COLOR_MAX][SQUARE_MAX];
U64 ep_[SQUARE_MAX];
U64 castling_[16];
U64 turn_;

U64 RandU64() {
  U64 r = rand();
  r <<= 32;
  r |= rand();
  return r;
}

}  // namespace

namespace zobrist {

U64 Get(Piece piece, int sq) {
  return zobrist_[PieceType(piece)][SideIndex(PieceSide(piece))][sq];
}

U64 Turn() {
  return turn_;
}

U64 EP(int sq) {
  return ep_[sq];
}

U64 Castling(unsigned index) {
  return castling_[index];
}

U64 Castling(unsigned char castle) {
  return castling_[castle];
}

void InitializeIfNeeded() {
  if (initialized) {
    return;
  }

  turn_ = RandU64();
  for (int i = 0; i < SQUARE_MAX; ++i) {
    ep_[i] = RandU64();
  }
  for (int i = 0; i < 16; ++i) {
    castling_[i] = RandU64();
  }
  for (int i = 0; i < PIECE_MAX; ++i) {
    for (int j = 0; j < COLOR_MAX; ++j) {
      for (int k = 0; k < SQUARE_MAX; ++k) {
        zobrist_[i][j][k] = RandU64();
      }
    }
  }
  initialized = true;
}

void PrintZobrist() {
  for (int i = 0; i < PIECE_MAX; ++i) {
    for (int j = 0; j < COLOR_MAX; ++j) {
      for (int k = 0; k < SQUARE_MAX; ++k) {
        std::cout << "# " << zobrist_[i][j][k] << std::endl;
      }
    }
  }
}

}  // namespace zobrist
