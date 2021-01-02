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

int SearchAlgorithm::NegaScout(int max_depth, int alpha, int beta,
                               SearchStats* search_stats) {
  return NegaScoutInternal(max_depth, alpha, beta, 0, true, search_stats);
}

int SearchAlgorithm::NegaScoutInternal(int max_depth, int alpha, int beta,
                                       int ply, bool allow_null_move,
                                       SearchStats* search_stats) {
  const U64 zkey = board_->ZobristKey();

  // Return DRAW if the position is repeated.
  for (int i = 4; i <= board_->HalfMoveClock(); i += 2) {
    if (zkey == board_->ZobristKey(i)) {
      return DRAW;
    }
  }

  TranspositionTableEntry* tentry = transpos_->Get(zkey);
  Move tt_move = Move();
  if (tentry != nullptr) {
    if (tentry->node_type == EXACT_NODE &&
        (tentry->score == WIN || tentry->score == -WIN)) {
      return tentry->score;
    }
    if (tentry->depth >= max_depth &&
        (tentry->node_type == EXACT_NODE ||
         (tentry->node_type == FAIL_HIGH_NODE && tentry->score >= beta) ||
         (tentry->node_type == FAIL_LOW_NODE && tentry->score <= alpha))) {
      return tentry->score;
    }
    tt_move = tentry->best_move;
  }

  if (max_depth == 0 || (timer_ && timer_->Lapsed())) {
    ++search_stats->nodes_evaluated;
    int score = evaluator_->Evaluate(alpha, beta);
    transpos_->Put(score, EXACT_NODE, 0, zkey, Move());
    return score;
  }

  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);

  // We have essentially reached the end of the game, so evaluate.
  if (move_array.size() == 0) {
    ++search_stats->nodes_evaluated;
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
                    !attacks::IsKingInCheck(*board_, board_->SideToMove());
  if (allow_null_move) {
    ++search_stats->nodes_searched;
    board_->MakeNullMove();
    int value = -NegaScoutInternal(max_depth - 2, -beta, -beta + 1, ply + 1,
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
    ++search_stats->nodes_searched;
    const MoveInfo& move_info = move_info_array.moves[index];
    const Move move = move_info.move;
    board_->MakeMove(move);

    int value = -INF;

    // Apply late move reduction if applicable.
    bool lmr_triggered = false;
    if (variant_ == Variant::ANTICHESS) {
      if (index >= 4 && max_depth >= 2) {
        value = -NegaScoutInternal(max_depth - 2, -alpha - 1, -alpha, ply + 1,
                                   true, search_stats);
        lmr_triggered = true;
      }
    }

    // If LMR was not triggered or LMR search failed high, proceed with normal
    // search.
    if (!lmr_triggered || value > alpha) {
      value = -NegaScoutInternal(max_depth - 1, -b, -alpha, ply + 1, true,
                                 search_stats);
    }

    // Re-search with wider window if null window fails high.
    if (value >= b && value < beta && index > 0 && max_depth > 1) {
      ++search_stats->nodes_researched;
      value = -NegaScoutInternal(max_depth - 1, -beta, -alpha, ply + 1, true,
                                 search_stats);
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