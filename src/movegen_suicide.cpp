#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "movegen_suicide.h"
#include "piece.h"
#include "u64_op.h"

#include <algorithm>
#include <array>

namespace {

void AddPawnMovesToMoveList(U64 pawns,
                            int add,
                            MoveArray* move_array) {
  static const std::array<Piece, 5> WHITE_PROMOTIONS =
      {ROOK, QUEEN, BISHOP, KNIGHT, KING};
  static const std::array<Piece, 5> BLACK_PROMOTIONS =
      {-ROOK, -QUEEN, -BISHOP, -KNIGHT, -KING};

  U64Op u64_op(pawns);
  int rmost_index;
  while ((rmost_index = u64_op.NextRightMostBitIndex()) != -1) {
    const int from = INDX((rmost_index + add) / 8, (rmost_index + add) % 8);
    Move move(from, rmost_index);
    if (rmost_index >= 56) {  // Pawn is in the 8th rank.
      AddPawnPromotions(WHITE_PROMOTIONS,
                        move.from_index(),
                        move.to_index(),
                        move_array);
    } else if (rmost_index <= 7) {  // Pawn is in the 1st rank.
      AddPawnPromotions(BLACK_PROMOTIONS,
                        move.from_index(),
                        move.to_index(),
                        move_array);
    } else {
      move_array->Add(move);
    }
  }
}

void GenerateAllPawnMoves(const Board& board,
                          MoveArray* move_array,
                          bool* capture_present) {
  switch (board.SideToMove()) {
  case Side::WHITE:
  {
    U64 pawns = board.BitBoard(PAWN);

    U64 enpassant = 0ULL;
    if (board.EnpassantTarget() != -1) {
      int index = board.EnpassantTarget();
      enpassant = (1ULL << index) & ~MaskRow(2);
    }

    // Check captures to north west.
    U64 pawns_1 = ((pawns & ~MaskColumn(0)) << 9) &
        (board.BitBoard(Side::BLACK) |
            enpassant);
    if (pawns_1) {
      *capture_present = true;
      AddPawnMovesToMoveList(pawns_1, -9, move_array);
    }

    // Check captures to north east.
    U64 pawns_2 = ((pawns & ~MaskColumn(7)) << 7) &
        (board.BitBoard(Side::BLACK) |
            enpassant);
    if (pawns_2) {
      *capture_present = true;
      AddPawnMovesToMoveList(pawns_2, -7, move_array);
    }

    if (!*capture_present) {
      // Push all pawns by 1 step.
      U64 pawns_3 = (pawns << 8) & ~board.BitBoard();
      if (pawns_3) {
        AddPawnMovesToMoveList(pawns_3, -8, move_array);
      }

      // Push rank 2 pawns by 2 steps.
      U64 pawns_4 = (pawns << 16) & ~board.BitBoard() & MaskRow(3);
      pawns_4 &= ~(board.BitBoard() & MaskRow(2)) << 8;
      if (pawns_4) {
        AddPawnMovesToMoveList(pawns_4, -16, move_array);
      }
    }
  }
  break;

  case Side::BLACK:
  {
    U64 pawns = board.BitBoard(-PAWN);

    U64 enpassant = 0ULL;
    if (board.EnpassantTarget() != -1) {
      int index = board.EnpassantTarget();
      enpassant = (1ULL << index) & ~MaskRow(5);
    }

    // Check captures to south west.
    U64 pawns_1 = ((pawns & ~MaskColumn(0)) >> 7) &
        (board.BitBoard(Side::WHITE) |
            enpassant);
    if (pawns_1) {
      *capture_present = true;
      AddPawnMovesToMoveList(pawns_1, 7, move_array);
    }

    // Check captures to south east.
    U64 pawns_2 = ((pawns & ~MaskColumn(7)) >> 9) &
        (board.BitBoard(Side::WHITE) |
            enpassant);
    if (pawns_2) {
      *capture_present = true;
      AddPawnMovesToMoveList(pawns_2, 9, move_array);
    }

    if (!*capture_present) {
      // Push all pawns by 1 step.
      U64 pawns_3 = (pawns >> 8) & ~board.BitBoard();
      if (pawns_3) {
        AddPawnMovesToMoveList(pawns_3, 8, move_array);
      }

      // Push rank 2 pawns by 2 steps.
      U64 pawns_4 = (pawns >> 16) & ~board.BitBoard() & MaskRow(4);
      pawns_4 &= ~(board.BitBoard() & MaskRow(5)) >> 8;
      if (pawns_4) {
        AddPawnMovesToMoveList(pawns_4, 16, move_array);
      }
    }
  }
  break;
  }
}

void GeneratePieceMoves(const Board& board,
                        Piece piece,
                        MoveArray* move_array,
                        bool* capture_present) {
  const Side side = board.SideToMove();
  U64Op u64_op(board.BitBoard(piece));
  int rmost_index;
  while ((rmost_index = u64_op.NextRightMostBitIndex()) != -1) {
    U64 attack_map = Attacks(piece, rmost_index, board);
    attack_map &= ~board.BitBoard(side);
    if (attack_map & board.BitBoard(OppositeSide(side))) {
      attack_map &= board.BitBoard(OppositeSide(side));
      *capture_present = true;
    }
    BitBoardToMoveList(rmost_index, attack_map, move_array);
  }
}

void GenerateMovesWithCaptures(const Board& board,
                               Piece piece,
                               MoveArray* move_array) {
  const Side side = board.SideToMove();
  U64Op u64_op(board.BitBoard(piece));
  int rmost_index;
  while ((rmost_index = u64_op.NextRightMostBitIndex()) != -1) {
    U64 attack_map = Attacks(piece, rmost_index, board) &
        ~board.BitBoard(side) & board.BitBoard(OppositeSide(side));
    BitBoardToMoveList(rmost_index, attack_map, move_array);
  }
}

}  // namespace

void MoveGeneratorSuicide::GenerateMoves(MoveArray* move_array) {
  const Side side = board_.SideToMove();
  int multiplier = (side == Side::WHITE) ? 1 : -1;
  Piece pawn = multiplier * PAWN;
  Piece bishop = multiplier * BISHOP;
  Piece knight = multiplier * KNIGHT;
  Piece rook = multiplier * ROOK;
  Piece queen = multiplier * QUEEN;
  Piece king = multiplier * KING;

  U64 bishop_captures = AllAttacks(bishop, board_) &
                        board_.BitBoard(OppositeSide(side));
  U64 knight_captures = AllAttacks(knight, board_) &
                        board_.BitBoard(OppositeSide(side));
  U64 rook_captures = AllAttacks(rook, board_) &
                      board_.BitBoard(OppositeSide(side));
  U64 queen_captures = AllAttacks(queen, board_) &
                       board_.BitBoard(OppositeSide(side));
  U64 king_captures = AllAttacks(king, board_) &
                      board_.BitBoard(OppositeSide(side));

  bool capture_present = (bishop_captures | knight_captures | rook_captures
      | queen_captures | king_captures);

  GenerateAllPawnMoves(board_, move_array, &capture_present);

  if (capture_present) {
    if (bishop_captures) {
      GenerateMovesWithCaptures(board_, bishop, move_array);
    }
    if (knight_captures) {
      GenerateMovesWithCaptures(board_, knight, move_array);
    }
    if (rook_captures) {
      GenerateMovesWithCaptures(board_, rook, move_array);
    }
    if (queen_captures) {
      GenerateMovesWithCaptures(board_, queen, move_array);
    }
    if (king_captures) {
      GenerateMovesWithCaptures(board_, king, move_array);
    }
  } else {
    GeneratePieceMoves(board_, bishop, move_array, &capture_present);
    GeneratePieceMoves(board_, knight, move_array, &capture_present);
    GeneratePieceMoves(board_, rook, move_array, &capture_present);
    GeneratePieceMoves(board_, queen, move_array, &capture_present);
    GeneratePieceMoves(board_, king, move_array, &capture_present);
  }
}

bool MoveGeneratorSuicide::IsValidMove(const Move& move) {
  MoveArray move_array;
  GenerateMoves(&move_array);
  return move_array.Find(move) != -1;
}
