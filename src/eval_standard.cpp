#include "eval_standard.h"
#include "attacks.h"
#include "board.h"
#include "common.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "piece.h"
#include "stopwatch.h"

namespace sc = standard_chess;
namespace pv = standard_chess::piece_value;

int EvalStandard::StaticEval() const {
  const Side side = board_->SideToMove();
  if (attacks::InCheck(*board_, side)) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    const int self_moves = move_array.size();
    if (self_moves == 0) {
      return -WIN;
    }
  }
  const Side opp_side = OppositeSide(side);
  if (attacks::InCheck(*board_, opp_side)) {
    return WIN;
  }

  const U64 w_king = board_->BitBoard(KING);
  const U64 w_queen = board_->BitBoard(QUEEN);
  const U64 w_rook = board_->BitBoard(ROOK);
  const U64 w_bishop = board_->BitBoard(BISHOP);
  const U64 w_knight = board_->BitBoard(KNIGHT);
  const U64 w_pawn = board_->BitBoard(PAWN);

  const U64 b_king = board_->BitBoard(-KING);
  const U64 b_queen = board_->BitBoard(-QUEEN);
  const U64 b_rook = board_->BitBoard(-ROOK);
  const U64 b_bishop = board_->BitBoard(-BISHOP);
  const U64 b_knight = board_->BitBoard(-KNIGHT);
  const U64 b_pawn = board_->BitBoard(-PAWN);

  int score = 0;

  score += PopCount(w_king) * pv::KING + sc::PSTScore<KING>(w_king);
  score += PopCount(w_queen) * pv::QUEEN + sc::PSTScore<QUEEN>(w_queen);
  score += PopCount(w_rook) * pv::ROOK + sc::PSTScore<ROOK>(w_rook);
  score += PopCount(w_bishop) * pv::BISHOP + sc::PSTScore<BISHOP>(w_bishop);
  score += PopCount(w_knight) * pv::KNIGHT + sc::PSTScore<KNIGHT>(w_knight);
  score += PopCount(w_pawn) * pv::PAWN + sc::PSTScore<PAWN>(w_pawn);

  score -= PopCount(b_king) * pv::KING + sc::PSTScore<-KING>(b_king);
  score -= PopCount(b_queen) * pv::QUEEN + sc::PSTScore<-QUEEN>(b_queen);
  score -= PopCount(b_rook) * pv::ROOK + sc::PSTScore<-ROOK>(b_rook);
  score -= PopCount(b_bishop) * pv::BISHOP + sc::PSTScore<-BISHOP>(b_bishop);
  score -= PopCount(b_knight) * pv::KNIGHT + sc::PSTScore<-KNIGHT>(b_knight);
  score -= PopCount(b_pawn) * pv::PAWN + sc::PSTScore<-PAWN>(b_pawn);

  if (side == Side::BLACK) {
    score = -score;
  }
  return score;
}

int EvalStandard::Quiesce(int alpha, int beta) {
  bool in_check = attacks::InCheck(*board_, board_->SideToMove());
  if (!in_check) {
    int standing_pat = StaticEval();
    if (standing_pat >= beta) {
      return standing_pat;
    }
    if (standing_pat > alpha) {
      alpha = standing_pat;
    }
  }
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  MoveInfoArray move_info_array;
  orderer_.Order(move_array, nullptr, &move_info_array);
  for (size_t i = 0; i < move_info_array.size; ++i) {
    const MoveInfo& move_info = move_info_array.moves[i];
    const Move move = move_info.move;
    if (in_check || move_info.type == MoveType::SEE_GOOD_CAPTURE) {
      board_->MakeMove(move);
      int score = -Quiesce(-beta, -alpha);
      board_->UnmakeLastMove();
      if (score >= beta) {
        return score;
      }
      if (score > alpha) {
        alpha = score;
      }
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
