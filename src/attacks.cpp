#include "attacks.h"
#include "bitmanip.h"
#include "board.h"
#include "common.h"
#include "magic-bits/include/magic_bits.hpp"

#include <cassert>
#include <cstdlib>
#include <tuple>

using std::vector;

namespace {

const magic_bits::Attacks magic_bits_attacks;

// KING and KNIGHT attacks

const auto knight_attacks = []() {
  std::array<U64, 64> knight_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      knight_attacks[INDX(i, j)] =
          (SetBit(i - 1, j - 2) | SetBit(i + 1, j - 2) | SetBit(i + 2, j - 1) |
           SetBit(i + 2, j + 1) | SetBit(i + 1, j + 2) | SetBit(i - 1, j + 2) |
           SetBit(i - 2, j + 1) | SetBit(i - 2, j - 1));
    }
  }
  return knight_attacks;
}();

const auto king_attacks = []() {
  std::array<U64, 64> king_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      king_attacks[INDX(i, j)] =
          (SetBit(i - 1, j - 1) | SetBit(i, j - 1) | SetBit(i + 1, j - 1) |
           SetBit(i + 1, j) | SetBit(i + 1, j + 1) | SetBit(i, j + 1) |
           SetBit(i - 1, j + 1) | SetBit(i - 1, j));
    }
  }
  return king_attacks;
}();

U64 RookAttacks(const U64 bitboard, const int index) {
  return magic_bits_attacks.Rook(bitboard, index);
}

U64 BishopAttacks(const U64 bitboard, const int index) {
  return magic_bits_attacks.Bishop(bitboard, index);
}

U64 QueenAttacks(const U64 bitboard, const int index) {
  return magic_bits_attacks.Queen(bitboard, index);
}

U64 KnightAttacks(const U64 unused_bitboard, const int index) {
  return knight_attacks[index];
}

U64 KingAttacks(const U64 unused_bitboard, const int index) {
  return king_attacks[index];
}

using AttacksFn = U64 (*)(const U64, const int);

AttacksFn attacks_fns[6] = {nullptr,     KingAttacks,   QueenAttacks,
                            RookAttacks, BishopAttacks, KnightAttacks};

template <Side side>
bool InCheckInternal(const Board& board) {
  const U64 king_bb = board.BitBoard(PieceOfSide(KING, side));
  if (!king_bb) {
    return false;
  }
  const int king_sq = Lsb1(king_bb);
  const U64 occ = board.BitBoard();
  constexpr Side opp_side = OppositeSide(side);
  const U64 rook = magic_bits_attacks.Rook(occ, king_sq);
  const U64 bishop = magic_bits_attacks.Bishop(occ, king_sq);
  return (rook & board.BitBoard(PieceOfSide(ROOK, opp_side))) ||
         (bishop & board.BitBoard(PieceOfSide(BISHOP, opp_side))) ||
         ((rook | bishop) & board.BitBoard(PieceOfSide(QUEEN, opp_side))) ||
         (knight_attacks[king_sq] &
          board.BitBoard(PieceOfSide(KNIGHT, opp_side))) ||
         ((bitmanip::siderel::PushNorthEast<side>(king_bb) |
           bitmanip::siderel::PushNorthWest<side>(king_bb)) &
          board.BitBoard(PieceOfSide(PAWN, opp_side)));
}

} // namespace

namespace attacks {

U64 Attacks(const U64 bitboard, const int index, const Piece piece) {
  return attacks_fns[PieceType(piece)](bitboard, index);
}

U64 SquareAttackers(const int square, const Piece attacking_piece,
                    const U64 occ, const U64 attacking_side_piece_occ) {
  const Side attacking_side = PieceSide(attacking_piece);
  const U64 bb = (1ULL << square);
  U64 attack_bb = 0ULL;
  if (PieceType(attacking_piece) == PAWN) {
    switch (attacking_side) {
    case Side::WHITE:
      attack_bb = bitmanip::siderel::PushNorthEast<Side::BLACK>(bb) |
                  bitmanip::siderel::PushNorthWest<Side::BLACK>(bb);
      break;
    case Side::BLACK:
      attack_bb = bitmanip::siderel::PushNorthEast<Side::WHITE>(bb) |
                  bitmanip::siderel::PushNorthWest<Side::WHITE>(bb);
      break;
    default:
      throw std::logic_error("cannot process side");
    }
  } else {
    attack_bb = Attacks(occ, square, attacking_piece);
  }
  return attack_bb & attacking_side_piece_occ;
}

bool InCheck(const Board& board, const Side side) {
  if (side == Side::BLACK) {
    return InCheckInternal<Side::BLACK>(board);
  }
  assert(side == Side::WHITE);
  return InCheckInternal<Side::WHITE>(board);
}

} // namespace attacks
