#include "zobrist.h"
#include "common.h"
#include "piece.h"

#include <array>
#include <iostream>
#include <random>

namespace {

constexpr int PIECE_MAX = 7;
constexpr int COLOR_MAX = 3;
constexpr int SQUARE_MAX = 64;

U64 RandU64() {
  static std::default_random_engine generator(832429);
  static std::uniform_int_distribution<uint64_t> distribution(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  return distribution(generator);
}

template <typename T, int A, int B, int C>
using Array3d = std::array<std::array<std::array<T, C>, B>, A>;

const Array3d<U64, PIECE_MAX, COLOR_MAX, SQUARE_MAX> zobrist_ = []() {
  Array3d<U64, PIECE_MAX, COLOR_MAX, SQUARE_MAX> zobrist;
  for (int i = 0; i < PIECE_MAX; ++i) {
    for (int j = 0; j < COLOR_MAX; ++j) {
      for (int k = 0; k < SQUARE_MAX; ++k) {
        zobrist[i][j][k] = RandU64();
      }
    }
  }
  return zobrist;
}();

const std::array<U64, SQUARE_MAX> ep_ = []() {
  std::array<U64, SQUARE_MAX> ep;
  for (int i = 0; i < SQUARE_MAX; ++i) {
    ep[i] = RandU64();
  }
  return ep;
}();

const std::array<U64, 16> castling_ = []() {
  std::array<U64, 16> castling;
  for (int i = 0; i < 16; ++i) {
    castling[i] = RandU64();
  }
  return castling;
}();

const U64 turn_ = RandU64();

} // namespace

namespace zobrist {

U64 Get(Piece piece, int sq) {
  return zobrist_[PieceType(piece)][SideIndex(PieceSide(piece))][sq];
}

U64 Turn() { return turn_; }

U64 EP(int sq) { return ep_[sq]; }

U64 Castling(unsigned index) { return castling_[index]; }

U64 Castling(unsigned char castle) { return castling_[castle]; }

void PrintZobrist() {
  for (int i = 0; i < PIECE_MAX; ++i) {
    for (int j = 0; j < COLOR_MAX; ++j) {
      for (int k = 0; k < SQUARE_MAX; ++k) {
        std::cout << "# " << zobrist_[i][j][k] << std::endl;
      }
    }
  }
}

} // namespace zobrist
