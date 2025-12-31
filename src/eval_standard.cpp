#include "attacks.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "params/params.h"
#include "pawns.h"
#include "pst.h"
#include "std_eval_params.h"
#include "std_static_eval.h"
#include "stopwatch.h"

#include <iostream>

namespace {

constexpr int FUTILITY_MARGIN = 50;

int StaticEval(Board& board) {
  static const StdEvalParams<int> params = BlessedParams();
  return standard::StaticEval(params, board);
}

} // namespace

template <Variant variant>
  requires(IsStandard(variant))
int Evaluate(Board& board, EGTB* egtb, int alpha, int beta) {
  assert(egtb == nullptr);
  bool in_check = attacks::InCheck(board, board.SideToMove());
  int standing_pat = StaticEval(board);
  if (!in_check) {
    if (standing_pat >= beta) {
      return standing_pat;
    }
    if (standing_pat > alpha) {
      alpha = standing_pat;
    }
  }
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);
  if (move_array.size() == 0) {
    return in_check ? -WIN : DRAW;
  }
  const MoveInfoArray move_info_array =
      OrderMoves<Variant::STANDARD>(board, move_array, nullptr);
  for (size_t i = 0; i < move_info_array.size; ++i) {
    const MoveInfo& move_info = move_info_array.moves[i];
    const Move move = move_info.move;
    if (in_check || move_info.type == MoveType::SEE_GOOD_CAPTURE) {
      board.MakeMove(move);
      if (!in_check && move_info.type == MoveType::SEE_GOOD_CAPTURE &&
          standing_pat + move_info.score + FUTILITY_MARGIN < alpha &&
          !attacks::InCheck(board, board.SideToMove())) {
        board.UnmakeLastMove();
        continue;
      }
      int score = -Evaluate<Variant::STANDARD>(board, egtb, -beta, -alpha);
      board.UnmakeLastMove();
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

template <Variant variant>
  requires(IsStandard(variant))
int EvalResult(Board& board) {
  const Side side = board.SideToMove();
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);
  if (move_array.size() == 0) {
    const U64 attack_map = ComputeAttackMap(board, OppositeSide(side));
    if (attack_map & board.BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW; // stalemate
    }
  }
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move move = move_array.get(i);
    if (board.PieceAt(move.to_index()) ==
        PieceOfSide(KING, OppositeSide(side))) {
      return WIN;
    }
  }
  return UNKNOWN;
}

template int Evaluate<Variant::STANDARD>(Board& board, EGTB* egtb, int alpha,
                                         int beta);
template int EvalResult<Variant::STANDARD>(Board& board);
