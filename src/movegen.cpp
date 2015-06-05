#include "board.h"
#include "common.h"
#include "move_array.h"
#include "movegen.h"
#include "piece.h"
#include "slider_attacks.h"
#include "u64_op.h"

#include <array>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

namespace {

constexpr U64 SetBit(int row, int col) {
  return IsOnBoard(row, col) ? (1ULL << INDX(row, col)) : 0ULL;
}

const std::array<U64, 64> knight_attacks_ = []() {
  std::array<U64, 64> knight_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      knight_attacks[INDX(i, j)] = (SetBit(i - 1, j - 2) |
                                    SetBit(i + 1, j - 2) |
                                    SetBit(i + 2, j - 1) |
                                    SetBit(i + 2, j + 1) |
                                    SetBit(i + 1, j + 2) |
                                    SetBit(i - 1, j + 2) |
                                    SetBit(i - 2, j + 1) |
                                    SetBit(i - 2, j - 1));
    }
  }
  return knight_attacks;
}();

const std::array<U64, 64> king_attacks_ = []() {
  std::array<U64, 64> king_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      king_attacks[INDX(i, j)] = (SetBit(i - 1, j - 1) |
                                  SetBit(i, j - 1) |
                                  SetBit(i + 1, j - 1) |
                                  SetBit(i + 1, j) |
                                  SetBit(i + 1, j + 1) |
                                  SetBit(i, j + 1) |
                                  SetBit(i - 1, j + 1) |
                                  SetBit(i - 1, j));
    }
  }
  return king_attacks;
}();

}  // namespace

void BitBoardToMoveList(const int index,
                        const U64 bitboard,
                        MoveArray* move_array) {
  U64Op u64_op(bitboard);
  int rmost_index;
  while ((rmost_index = u64_op.NextRightMostBitIndex()) != -1) {
    move_array->Add(Move(index, rmost_index));
  }
}

U64 ComputeAttackMap(const Board& board, const Side attacker_side) {
  int multiplier = (attacker_side == Side::WHITE) ? 1 : -1;

  const Piece king = multiplier * KING;
  const Piece queen = multiplier * QUEEN;
  const Piece rook = multiplier * ROOK;
  const Piece bishop = multiplier * BISHOP;
  const Piece knight = multiplier * KNIGHT;
  const Piece pawn = multiplier * PAWN;

  const U64 kings = AllAttacks(king, board);
  const U64 queens = AllAttacks(queen, board);
  const U64 rooks = AllAttacks(rook, board);
  const U64 bishops = AllAttacks(bishop, board);
  const U64 knights = AllAttacks(knight, board);
  const U64 pawns = AllPawnCaptures(attacker_side, board);

  // We ignore attacks on self pieces. Including self piece
  // attacks allows us to avoid invalid captures when king
  // moves in normal chess.
  const U64 attack_map = (kings | queens | rooks | bishops | knights | pawns);
  return attack_map;
}

U64 Attacks(const Piece piece, const int index, const Board& board) {
  switch (PieceType(piece)) {
    case BISHOP:
      return slider_attacks::BishopAttacks(board.BitBoard(), index);

    case KING:
      return king_attacks_[index];

    case KNIGHT:
      return knight_attacks_[index];

    case QUEEN:
      return slider_attacks::QueenAttacks(board.BitBoard(), index);

    case ROOK:
      return slider_attacks::RookAttacks(board.BitBoard(), index);
  }
  throw std::runtime_error("Unknown piece type");
}

U64 AllAttacks(const Piece piece, const Board& board) {
  U64 attack_map = 0ULL;
  U64 bb = board.BitBoard(piece);
  while (bb) {
    U64 rmost = bb & -bb;
    U64 rmost_index = log2U(rmost);
    attack_map |= Attacks(piece, rmost_index, board);
    bb ^= rmost;
  }
  return attack_map;
}

U64 AllPawnCaptures(const Side side, const Board& board) {
  U64 attack_map = 0ULL;
  Piece pawn = PAWN;
  if (side == Side::BLACK) {
    pawn = -PAWN;
  }
  const U64 pawns_board = board.BitBoard(pawn);

  if (side == Side::BLACK) {
    U64 temp = pawns_board;
    temp &= ~MaskColumn(0);
    temp >>= 7;
    attack_map |= temp;

    temp = pawns_board;
    temp &= ~MaskColumn(7);
    temp >>= 9;
    attack_map |= temp;
  } else {
    U64 temp = pawns_board;
    temp &= ~MaskColumn(0);
    temp <<= 9;
    attack_map |= temp;

    temp = pawns_board;
    temp &= ~MaskColumn(7);
    temp <<= 7;
    attack_map |= temp;
  }
  return attack_map;
}

int CountMoves(const Side side, const Board& board) {
  int multiplier = 1;
  Side opp_side = Side::BLACK;
  if (side == Side::BLACK) {
    multiplier = -1;
    opp_side = Side::WHITE;
  }

  static const Piece pieces[] = { KING, QUEEN, ROOK, BISHOP, KNIGHT };

  int attacks_count = 0;
  int captures_count = 0;
  for (int i = 0; i < 5; ++i) {
    Piece piece = multiplier * pieces[i];
    U64 pboard = board.BitBoard(piece);
    while (pboard) {
      U64 rmost = pboard & -pboard;
      U64 rmost_index = log2U(rmost);
      U64 attacks = Attacks(piece, rmost_index, board) & ~board.BitBoard(side);
      U64 captures = attacks & board.BitBoard(opp_side);
      captures_count += PopCount(captures);
      if (!captures_count) {
        attacks_count += PopCount(attacks);
      }
      pboard ^= rmost;
    }
  }

  Piece pawn = multiplier * PAWN;
  U64 pawn_board = board.BitBoard(pawn);

  switch (side) {
  case Side::WHITE:
  {
    U64 enpassant = 0ULL;
    if (board.EnpassantTarget() != -1) {
      int index = board.EnpassantTarget();
      enpassant = (1ULL << index) & ~MaskRow(2);
    }

    // Check captures to north west.
    U64 pawns_1 = ((pawn_board & ~MaskColumn(0)) << 9) &
        ~board.BitBoard(Side::WHITE) &
        (board.BitBoard(Side::BLACK) |
            enpassant);
    if (pawns_1) {
      if (pawns_1 & MaskRow(7)) {
        captures_count += 5 * PopCount(pawns_1 & MaskRow(7));
      }
      captures_count += PopCount(pawns_1 & ~MaskRow(7));
    }

    // Check captures to north east.
    U64 pawns_2 = ((pawn_board & ~MaskColumn(7)) << 7) &
        ~board.BitBoard(Side::WHITE) &
        (board.BitBoard(Side::BLACK) |
            enpassant);
    if (pawns_2) {
      if (pawns_2 & MaskRow(7)) {
        captures_count += 5 * PopCount(pawns_2 & MaskRow(7));
      }
      captures_count += PopCount(pawns_2 & ~MaskRow(7));
    }

    if (!captures_count) {
      // Push all pawns by 1 step.
      U64 pawns_3 = (pawn_board << 8) & ~board.BitBoard();
      if (pawns_3) {
        if (pawns_3 & MaskRow(7)) {
          attacks_count += 5 * PopCount(pawns_3 & MaskRow(7));
        }
        attacks_count += PopCount(pawns_3 & ~MaskRow(7));
      }

      // Push rank 2 pawns by 2 steps.
      U64 pawns_4 = (pawn_board << 16) & ~board.BitBoard() & MaskRow(3);
      pawns_4 &= ~(board.BitBoard() & MaskRow(2)) << 8;
      if (pawns_4) {
        attacks_count += PopCount(pawns_4);
      }
    }
  }
  break;

  case Side::BLACK:
  {
    U64 enpassant = 0ULL;
    if (board.EnpassantTarget() != -1) {
      int index = board.EnpassantTarget();
      enpassant = (1ULL << index) & ~MaskRow(5);
    }

    // Check captures to south west.
    U64 pawns_1 = ((pawn_board & ~MaskColumn(0)) >> 7) &
        ~board.BitBoard(Side::BLACK) &
        (board.BitBoard(Side::WHITE) |
            enpassant);
    if (pawns_1) {
      if (pawns_1 & MaskRow(0)) {
        captures_count += 5 * PopCount(pawns_1 & MaskRow(0));
      }
      captures_count += PopCount(pawns_1 & ~MaskRow(0));
    }

    // Check captures to south east.
    U64 pawns_2 = ((pawn_board & ~MaskColumn(7)) >> 9) &
        ~board.BitBoard(Side::BLACK) &
        (board.BitBoard(Side::WHITE) |
            enpassant);
    if (pawns_2) {
      if (pawns_2 & MaskRow(0)) {
        captures_count += 5 * PopCount(pawns_2 & MaskRow(0));
      }
      captures_count += PopCount(pawns_2 & ~MaskRow(0));
    }

    if (!captures_count) {
      // Push all pawns by 1 step.
      U64 pawns_3 = (pawn_board >> 8) & ~board.BitBoard();
      if (pawns_3) {
        if (pawns_3 & MaskRow(0)) {
          attacks_count += 5 * PopCount(pawns_3 & MaskRow(0));
        }
        attacks_count += PopCount(pawns_3 & ~MaskRow(0));
      }

      // Push rank 2 pawns by 2 steps.
      U64 pawns_4 = (pawn_board >> 16) & ~board.BitBoard() & MaskRow(4);
      pawns_4 &= ~(board.BitBoard() & MaskRow(5)) >> 8;
      if (pawns_4) {
        attacks_count += PopCount(pawns_4);
      }
    }
  }
  break;
  }

  if (captures_count) {
    return captures_count;
  } else {
    return attacks_count;
  }
}
