#include "eval_standard.h"
#include "attacks.h"
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

int EvalStandard::StaticEval() const {
  const Side side = board_->SideToMove();
  if (attacks::IsKingInCheck(*board_, side)) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    const int self_moves = move_array.size();
    if (self_moves == 0) {
      return -WIN;
    }
  }
  const Side opp_side = OppositeSide(side);
  if (attacks::IsKingInCheck(*board_, opp_side)) {
    return WIN;
  }

  int score = PieceValDifference();

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

int EvalStandard::Quiesce(int alpha, int beta) {
  // TODO: Handle checks.
  int standing_pat = StaticEval();
  if (standing_pat >= beta) {
    return standing_pat;
  }
  if (standing_pat > alpha) {
    alpha = standing_pat;
  }
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  orderer_.Order(&move_array);
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move& move = move_array.get(i);
    if (board_->PieceAt(move.to_index()) != NULLPIECE) {
      board_->MakeMove(move);
      int score = -Quiesce(-beta, -alpha);
      board_->UnmakeLastMove();
      if (score >= beta) {
        return score;
      }
      if (score > alpha) {
        alpha = score;
      }
    } else {
      break;
    }
  }
  return alpha;
}

int EvalStandard::Evaluate(int alpha, int beta) { return Quiesce(alpha, beta); }

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
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move move = move_array.get(i);
    if (board_->PieceAt(move.to_index()) ==
        PieceOfSide(KING, OppositeSide(side))) {
      return WIN;
    }
  }
  return UNKNOWN;
}
