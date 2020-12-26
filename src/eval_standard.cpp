#include "eval_standard.h"
#include "board.h"
#include "common.h"
#include "movegen.h"
#include "piece.h"
#include "stopwatch.h"

namespace {
// clang-format off
const int PSTPawn[64] = {
   0,  0,   0,   0,   0,   0,  0,  0,
  50, 50,  50,  50,  50,  50, 50, 50,
  10, 10,  20,  30,  30,  20, 10, 10,
   5,  5,  10,  25,  25,  10,  5,  5,
   0,  0,   0,  20,  20,   0,  0,  0,
   5, -5, -10,   0,   0, -10, -5,  5,
   5, 10,  10, -20, -20,  10, 10,  5,
   0,  0,   0,   0,   0,   0,  0,  0
};
// clang-format on
} // namespace

int EvalStandard::PieceValDifference() const {
  namespace pv = standard_chess::piece_value;
  const int white_val = PopCount(board_->BitBoard(KING)) * pv::KING +
                        PopCount(board_->BitBoard(QUEEN)) * pv::QUEEN +
                        PopCount(board_->BitBoard(PAWN)) * pv::PAWN +
                        PopCount(board_->BitBoard(BISHOP)) * pv::BISHOP +
                        PopCount(board_->BitBoard(KNIGHT)) * pv::KNIGHT +
                        PopCount(board_->BitBoard(ROOK)) * pv::ROOK;
  const int black_val = PopCount(board_->BitBoard(-KING)) * pv::KING +
                        PopCount(board_->BitBoard(-QUEEN)) * pv::QUEEN +
                        PopCount(board_->BitBoard(-PAWN)) * pv::PAWN +
                        PopCount(board_->BitBoard(-BISHOP)) * pv::BISHOP +
                        PopCount(board_->BitBoard(-KNIGHT)) * pv::KNIGHT +
                        PopCount(board_->BitBoard(-ROOK)) * pv::ROOK;
  return white_val - black_val;
}

int EvalStandard::Evaluate(int alpha, int beta) {
  const Side side = board_->SideToMove();
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  if (move_array.size() == 0) {
    const U64 attack_map = ComputeAttackMap(*board_, OppositeSide(side));
    if (attack_map & board_->BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW; // stalemate
    }
  }
  int score = 0;
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move& move = move_array.get(i);
    board_->MakeMove(move);
    U64 attack_map = ComputeAttackMap(*board_, side);
    if (attack_map & board_->BitBoard(PieceOfSide(KING, OppositeSide(side)))) {
      MoveArray opp_move_array;
      movegen_->GenerateMoves(&opp_move_array);
      if (opp_move_array.size() == 0) {
        score = WIN;
      }
    }
    board_->UnmakeLastMove();
  }
  if (score == WIN) {
    return score;
  }

  score += PieceValDifference();

  U64 w_pawns = board_->BitBoard(PieceOfSide(PAWN, Side::WHITE));
  while (w_pawns) {
    const int lsb_index = Lsb1(w_pawns);
    const int sq = INDX(7 - ROW(lsb_index), COL(lsb_index));
    score += PSTPawn[sq];
    w_pawns ^= (1ULL << lsb_index);
  }

  U64 b_pawns = board_->BitBoard(PieceOfSide(PAWN, Side::BLACK));
  while (b_pawns) {
    const int lsb_index = Lsb1(b_pawns);
    score -= PSTPawn[lsb_index];
    b_pawns ^= (1ULL << lsb_index);
  }

  if (board_->SideToMove() == Side::BLACK) {
    score = -score;
  }
  return score;
}

int EvalStandard::Result() const {
  const Side side = board_->SideToMove();
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  if (move_array.size() == 0) {
    const U64 attack_map = ComputeAttackMap(*board_, OppositeSide(side));
    if (attack_map & board_->BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW; // stalemate
    }
  }
  return -1;
}
