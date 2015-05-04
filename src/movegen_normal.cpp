#include <algorithm>
#include <cstdlib>

#include "board.h"
#include "common.h"
#include "movegen.h"
#include "movegen_normal.h"
#include "piece.h"
#include "move.h"
#include "u64_op.h"

namespace movegen {

namespace {

void AddPawnMovesToMoveList(U64 pawns, int add, MoveArray* move_array) {
  static const std::array<Piece, 4> WHITE_PROMOTIONS =
      {ROOK, QUEEN, BISHOP, KNIGHT};
  static const std::array<Piece, 4> BLACK_PROMOTIONS =
      {-ROOK, -QUEEN, -BISHOP, -KNIGHT};

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
                          MoveArray* move_array) {
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
      AddPawnMovesToMoveList(pawns_1, -9, move_array);
    }

    // Check captures to north east.
    U64 pawns_2 = ((pawns & ~MaskColumn(7)) << 7) &
        (board.BitBoard(Side::BLACK) |
            enpassant);
    if (pawns_2) {
      AddPawnMovesToMoveList(pawns_2, -7, move_array);
    }

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
      AddPawnMovesToMoveList(pawns_1, 7, move_array);
    }

    // Check captures to south east.
    U64 pawns_2 = ((pawns & ~MaskColumn(7)) >> 9) &
        (board.BitBoard(Side::WHITE) |
            enpassant);
    if (pawns_2) {
      AddPawnMovesToMoveList(pawns_2, 9, move_array);
    }

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
  break;
  }
}

bool IsSquareUnderAttack(const Board& board,
                         Side player_side,
                         int pos) {
  U64 attack_map = ComputeAttackMap(board, OppositeSide(player_side));
  if (attack_map & (1ULL << pos)) {
    return true;
  }
  return false;
}

void GeneratePieceMoves(const Board& board,
                        Piece piece,
                        MoveArray* move_array) {
  const Side side = board.SideToMove();
  U64Op u64_op(board.BitBoard(piece));
  int rmost_index;
  while ((rmost_index = u64_op.NextRightMostBitIndex()) != -1) {
    U64 attack_map = Attacks(piece, rmost_index, board);
    attack_map &= ~board.BitBoard(side);
    BitBoardToMoveList(rmost_index, attack_map, move_array);
  }
}

}  // namespace

void MoveGeneratorNormal::GenerateMoves(MoveArray* move_array) {
  const Side side = board_->SideToMove();
  int multiplier = (side == Side::WHITE) ? 1 : -1;

  const Piece pawn = multiplier * PAWN;
  const Piece bishop = multiplier * BISHOP;
  const Piece knight = multiplier * KNIGHT;
  const Piece rook = multiplier * ROOK;
  const Piece queen = multiplier * QUEEN;
  const Piece king = multiplier * KING;

  // Handle castling.
  if (board_->CanCastle(KING) || board_->CanCastle(QUEEN)) {
    const int row = (side == Side::WHITE) ? 0 : 7;
    const int b_sq = INDX(row, 1);
    const int c_sq = INDX(row, 2);
    const int d_sq = INDX(row, 3);
    const int e_sq = INDX(row, 4);
    const int f_sq = INDX(row, 5);
    const int g_sq = INDX(row, 6);

    // Returns true if a square is empty.
    auto is_empty = [&](const int sq) -> bool {
      return board_->PieceAt(sq) == NULLPIECE;
    };

    auto is_sq_attacked = [](const U64 attack_map, const int sq)->bool {
      return attack_map & (1ULL << sq);
    };

    if (board_->CanCastle(KING) && is_empty(f_sq) && is_empty(g_sq)) {
      const U64 attack_map = ComputeAttackMap(*board_, OppositeSide(side));
      if (!is_sq_attacked(attack_map, e_sq) &&
          !is_sq_attacked(attack_map, f_sq) &&
          !is_sq_attacked(attack_map, g_sq)) {
        move_array->Add(Move(e_sq, g_sq));
      }
    } else if (board_->CanCastle(QUEEN) && is_empty(d_sq) &&
               is_empty(c_sq) && is_empty(b_sq)) {
      const U64 attack_map = ComputeAttackMap(*board_, OppositeSide(side));
      if (!is_sq_attacked(attack_map, e_sq) &&
          !is_sq_attacked(attack_map, d_sq) &&
          !is_sq_attacked(attack_map, c_sq)) {
        move_array->Add(Move(e_sq, c_sq));
      }
    }
  }

  GeneratePieceMoves(*board_, bishop, move_array);
  GeneratePieceMoves(*board_, knight, move_array);
  GeneratePieceMoves(*board_, rook, move_array);
  GeneratePieceMoves(*board_, queen, move_array);
  GeneratePieceMoves(*board_, king, move_array);
  GenerateAllPawnMoves(*board_, move_array);

  // Filter out illegal moves.
  MoveArray legal_move_array;

  U64 before_move_attack_map = ComputeAttackMap(*board_, OppositeSide(side));
  if (before_move_attack_map & board_->BitBoard(king)) {  // King is under check
    for (int i = 0; i < move_array->size(); ++i) {
      const Move& move = move_array->get(i);
      board_->MakeMove(move);
      U64 after_move_attack_map =
          ComputeAttackMap(*board_, OppositeSide(side));
      if (!(after_move_attack_map & board_->BitBoard(king))) {
        legal_move_array.Add(move);
      }
      board_->UnmakeLastMove();
    }
  } else {
    int king_index = U64Op(board_->BitBoard(king)).NextRightMostBitIndex();
    if (king_index == -1) {
      board_->DebugPrintBoard();
    }
    assert(king_index != -1);
    // Find the pieces that could be pinned.
    U64 pinned_bb = (Attacks(QUEEN, king_index, *board_) &
                     before_move_attack_map & board_->BitBoard(side));
    for (int i = 0; i < move_array->size(); ++i) {
      const Move& move = move_array->get(i);
      // As long as it is not the king moving and none of the potentially pinned
      // pieces moving, add the move to legal moves without further checking.
      if (move.from_index() != king_index &&
          !((1ULL << move.from_index()) & pinned_bb)) {
        legal_move_array.Add(move);
        continue;
      }
      board_->MakeMove(move);
      U64 after_move_attack_map =
          ComputeAttackMap(*board_, OppositeSide(side));
      if (!(after_move_attack_map & board_->BitBoard(king))) {
        legal_move_array.Add(move);
      }
      board_->UnmakeLastMove();
    }
  }
  *move_array = legal_move_array;
}

bool MoveGeneratorNormal::IsValidMove(const Move& move) {
  MoveArray move_array;
  GenerateMoves(&move_array);
  return move_array.Find(move) != -1;
}

}  // namespace movegen
