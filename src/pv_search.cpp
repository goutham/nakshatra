#include "pv_search.h"
#include "attacks.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "move.h"
#include "move_order.h"
#include "movegen.h"
#include "stats.h"
#include "std_static_eval.h"
#include "timer.h"
#include "transpos.h"

namespace {

// Probes TT. Returns true if tt_score can be returned as the result of search
// at given max_depth.
bool Probe(int max_depth, int alpha, int beta, U64 zkey,
           TranspositionTable& transpos, int& tt_score, Move& tt_move) {
  const std::optional<TTData> tdata = transpos.Get(zkey);
  if (!tdata) {
    return false;
  }
  tt_score = tdata->score;
  tt_move = tdata->best_move;
  const NodeType node_type = tdata->node_type();
  if (node_type == NodeType::EXACT_NODE &&
      (tt_score == WIN || tt_score == -WIN)) {
    return true;
  }
  if (tdata->depth >= max_depth &&
      (node_type == NodeType::EXACT_NODE ||
       (node_type == NodeType::FAIL_HIGH_NODE && tt_score >= beta) ||
       (node_type == NodeType::FAIL_LOW_NODE && tt_score <= alpha))) {
    return true;
  }
  return false;
}
} // namespace

template <Variant variant>
int PVSearch<variant>::Search(int max_depth, int alpha, int beta,
                              SearchStats& search_stats) {
  return PVS(max_depth, alpha, beta, 0, true, search_stats);
}

template <Variant variant>
int PVSearch<variant>::PVS(int max_depth, int alpha, int beta, int ply,
                           bool allow_null_move, SearchStats& search_stats) {
  ++search_stats.nodes_searched;
  const U64 zkey = board_.ZobristKey();

  // Return DRAW if the position is repeated.
  for (int i = 4; i <= board_.HalfMoveClock(); i += 2) {
    if (zkey == board_.ZobristKey(i)) {
      return DRAW;
    }
  }

  if (max_depth <= 0 || (timer_ && timer_->Lapsed())) {
    return Evaluate<variant>(board_, egtb_, alpha, beta);
  }

  Move tt_move = Move();
  int tt_score = 0;
  if (Probe(max_depth, alpha, beta, zkey, transpos_, tt_score, tt_move)) {
    return tt_score;
  }
  // Search to a reduced depth to get a good first move to try (internal
  // iterative deepening).
  if (!tt_move.is_valid() && max_depth > 3) {
    PVS(max_depth - 3, alpha, beta, ply, allow_null_move, search_stats);
    if (Probe(max_depth, alpha, beta, zkey, transpos_, tt_score, tt_move)) {
      return tt_score;
    }
  }

  const bool in_check = attacks::InCheck(board_, board_.SideToMove());
  int static_eval = INF;

  if constexpr (!IsAntichessLike(variant)) {
    if (!in_check) {
      static_eval = StaticEval(board_);

      // Reverse Futility Pruning
      if (max_depth <= 3) {
        if (static_eval - max_depth * 100 >= beta) {
          return static_eval;
        }
      }

      // Razoring
      if (max_depth <= 1 && static_eval + 450 <= alpha) {
        return Evaluate<variant>(board_, egtb_, alpha, beta);
      }
    }

    // Decide whether to use null move pruning. Disabled for ANTICHESS where
    // zugzwangs are common.
    allow_null_move = allow_null_move && max_depth >= 2 && beta < INF &&
                      PopCount(board_.BitBoard()) > 10 && !in_check;
    if (allow_null_move) {
      board_.MakeNullMove();
      int R = 2;
      if (max_depth > 6) {
        R = 3;
      }
      int value = -PVS(max_depth - R, -beta, -beta + 1, ply + 1,
                       !allow_null_move, search_stats);
      board_.UnmakeNullMove();
      if (value >= beta) {
        return beta;
      }
    }
  }

  MoveArray move_array = GenerateMoves<variant>(board_);

  // We have essentially reached the end of the game, so evaluate.
  if (move_array.size() == 0) {
    return Evaluate<variant>(board_, egtb_, alpha, beta);
  }

  PrefMoves pref_moves;
  pref_moves.tt_move = tt_move;
  pref_moves.killer1 = killers_[ply][0];
  pref_moves.killer2 = killers_[ply][1];
  const MoveInfoArray move_info_array =
      OrderMoves<variant>(board_, move_array, &pref_moves);

  Move best_move;
  NodeType node_type = NodeType::FAIL_LOW_NODE;
  int b = beta;
  int score = -INF;
  for (size_t index = 0; index < move_info_array.size; ++index) {
    const MoveInfo& move_info = move_info_array.moves[index];
    const Move move = move_info.move;
    board_.MakeMove(move);

    int value = -INF;

    if constexpr (!IsAntichessLike(variant)) {
      // Futility Pruning
      // Prune quiet moves at low depth if they are unlikely to improve alpha.
      // We check if the move gives check (which is dangerous to prune) after MakeMove.
      if (!in_check && max_depth <= 3 && !move.is_promotion() &&
          move_info.type == MoveType::QUIET &&
          static_eval + max_depth * 200 <= alpha &&
          !attacks::InCheck(board_, board_.SideToMove())) {
        board_.UnmakeLastMove();
        continue;
      }
    }

    // Apply late move reduction if applicable.
    bool lmr_triggered = false;
    if (index >= 4 && max_depth >= 2) {
      value =
          -PVS(max_depth - 2, -alpha - 1, -alpha, ply + 1, true, search_stats);
      lmr_triggered = true;
    }

    // If LMR was not triggered or LMR search failed high, proceed with normal
    // search.
    if (!lmr_triggered || value > alpha) {
      value = -PVS(max_depth - 1, -b, -alpha, ply + 1, true, search_stats);
    }

    // Re-search with wider window if null window fails high.
    if (value >= b && value < beta && index > 0 && max_depth > 1) {
      value = -PVS(max_depth - 1, -beta, -alpha, ply + 1, true, search_stats);
    }

    board_.UnmakeLastMove();

    if (value > score) {
      score = value;
      if (score > alpha) {
        best_move = move;
        node_type = NodeType::EXACT_NODE;
        alpha = score;
        if (alpha >= beta) {
          node_type = NodeType::FAIL_HIGH_NODE;
          if (move != tt_move && move != killers_[ply][0] &&
              (IsAntichessLike(variant) ||
               board_.PieceAt(move.to_index()) == NULLPIECE)) {
            killers_[ply][1] = killers_[ply][0];
            killers_[ply][0] = move;
          }
          break;
        }
      }
    }
    b = alpha + 1;
  }

  if (!(timer_ && timer_->Lapsed())) {
    transpos_.Put(score, node_type, max_depth, zkey, best_move);
  }
  return score;
}

template class PVSearch<Variant::STANDARD>;
template class PVSearch<Variant::ANTICHESS>;
template class PVSearch<Variant::SUICIDE>;
