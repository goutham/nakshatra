#include "search_algorithm.h"
#include "attacks.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "extensions.h"
#include "move.h"
#include "move_order.h"
#include "movegen.h"
#include "stats.h"
#include "timer.h"
#include "transpos.h"

int SearchAlgorithm::Search(int max_depth, int alpha, int beta,
                            SearchStats* search_stats) {
  return NegaScout(max_depth, alpha, beta, 0, true, search_stats);
}

int SearchAlgorithm::NegaScout(int max_depth, int alpha, int beta, int ply,
                               bool allow_null_move,
                               SearchStats* search_stats) {
  ++search_stats->nodes_searched;
  const U64 zkey = board_->ZobristKey();

  // Return DRAW if the position is repeated.
  for (int i = 4; i <= board_->HalfMoveClock(); i += 2) {
    if (zkey == board_->ZobristKey(i)) {
      return DRAW;
    }
  }

  if (max_depth <= 0 || (timer_ && timer_->Lapsed())) {
    return evaluator_->Evaluate(alpha, beta);
  }

  Move tt_move = Move();
  if (TTEntry* tentry = transpos_->Get(zkey); tentry != nullptr) {
    const NodeType node_type = tentry->node_type();
    const int score = tentry->score;
    if (node_type == EXACT_NODE && (score == WIN || score == -WIN)) {
      return score;
    }
    if (tentry->depth >= max_depth &&
        (node_type == EXACT_NODE ||
         (node_type == FAIL_HIGH_NODE && score >= beta) ||
         (node_type == FAIL_LOW_NODE && score <= alpha))) {
      return score;
    }
    tt_move = tentry->best_move;
  }

  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);

  // We have essentially reached the end of the game, so evaluate.
  if (move_array.size() == 0) {
    return evaluator_->Evaluate(alpha, beta);
  }

  MoveInfoArray move_info_array;
  PrefMoves pref_moves;
  pref_moves.tt_move = tt_move;
  pref_moves.killer1 = killers_[ply][0];
  pref_moves.killer2 = killers_[ply][1];
  move_orderer_->Order(move_array, &pref_moves, &move_info_array);

  // Decide whether to use null move pruning. Disabled for ANTICHESS where
  // zugzwangs are common.
  allow_null_move = variant_ != Variant::ANTICHESS && allow_null_move &&
                    max_depth >= 2 && beta < INF &&
                    PopCount(board_->BitBoard()) > 10 &&
                    !attacks::InCheck(*board_, board_->SideToMove());
  if (allow_null_move) {
    board_->MakeNullMove();
    int value = -NegaScout(max_depth - 2, -beta, -beta + 1, ply + 1,
                           !allow_null_move, search_stats);
    board_->UnmakeNullMove();
    if (value >= beta) {
      return beta;
    }
  }

  Move best_move;
  NodeType node_type = FAIL_LOW_NODE;
  int b = beta;
  for (size_t index = 0; index < move_info_array.size; ++index) {
    const MoveInfo& move_info = move_info_array.moves[index];
    const Move move = move_info.move;
    board_->MakeMove(move);

    int value = -INF;

    // Apply late move reduction if applicable.
    bool lmr_triggered = false;
    if (variant_ == Variant::ANTICHESS) {
      if (index >= 4 && max_depth >= 2) {
        value = -NegaScout(max_depth - 2, -alpha - 1, -alpha, ply + 1, true,
                           search_stats);
        lmr_triggered = true;
      }
    }

    // If LMR was not triggered or LMR search failed high, proceed with normal
    // search.
    if (!lmr_triggered || value > alpha) {
      value =
          -NegaScout(max_depth - 1, -b, -alpha, ply + 1, true, search_stats);
    }

    // Re-search with wider window if null window fails high.
    if (value >= b && value < beta && index > 0 && max_depth > 1) {
      value =
          -NegaScout(max_depth - 1, -beta, -alpha, ply + 1, true, search_stats);
    }

    board_->UnmakeLastMove();

    if (value > alpha) {
      best_move = move;
      node_type = EXACT_NODE;
      alpha = value;
    }

    if (alpha >= beta) {
      node_type = FAIL_HIGH_NODE;
      if (move != tt_move && move != killers_[ply][0] &&
          (variant_ == Variant::ANTICHESS ||
           board_->PieceAt(move.to_index()) == NULLPIECE)) {
        killers_[ply][1] = killers_[ply][0];
        killers_[ply][0] = move;
      }
      break;
    }

    b = alpha + 1;
  }

  if (!timer_ || !timer_->Lapsed()) {
    transpos_->Put(alpha, node_type, max_depth, zkey, best_move);
  }
  return alpha;
}
