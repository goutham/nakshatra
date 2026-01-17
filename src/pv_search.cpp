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

#include <cstring>

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

template <Variant variant>
int LMRDepthReduction(int max_depth, int move_index, const MoveInfo& move_info) {
  if constexpr (IsAntichessLike(variant)) {
    return max_depth >= 2 && move_index >= 4 ? 2 : 0;
  }
  if constexpr (IsStandard(variant)) {
    int lmr_depth_reduction = 0;
    if (move_index >= 4 && max_depth >= 2 && move_info.type >= MoveType::KILLER) {
      lmr_depth_reduction = 2;
    }
    if (move_index >= 2 && max_depth >= 2 && move_info.type >= MoveType::QUIET) {
      lmr_depth_reduction = 2;
    }
    if (move_index >= 5 && max_depth >= 3 && move_info.type >= MoveType::QUIET) {
      lmr_depth_reduction = 3;
    }
    if (move_index >= 1 && max_depth >= 3 && move_info.type >= MoveType::SEE_BAD_CAPTURE) {
      lmr_depth_reduction = 3;
    }
    return lmr_depth_reduction;
  }
  throw std::logic_error("unsupported variant");
}
} // namespace

template <Variant variant>
void PVSearch<variant>::UpdateHistory(const Move& move, int depth, bool is_bonus) {
  const int MAX_HISTORY = 16384;
  const int bonus = std::min(depth * depth, 1200);
  const int side_idx = SideIndex(board_.SideToMove());
  const int from = move.from_index();
  const int to = move.to_index();
  int& val = history_[side_idx][from][to];

  if (is_bonus) {
    val += bonus - (val * bonus / MAX_HISTORY);
  } else {
    val -= bonus + (val * bonus / MAX_HISTORY);
  }
}

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
  if constexpr (!IsAntichessLike(variant)) {
    // Are we likely to be too good already to bother searching this position?
    if (!in_check && (max_depth == 1 || max_depth == 2)) {
      const int eval_score = StaticEval(board_);
      if (eval_score - max_depth * 75 >= beta) {
        return eval_score;
      }
    }

    // Decide whether to use null move pruning. Disabled for ANTICHESS where
    // zugzwangs are common.
    allow_null_move = allow_null_move && max_depth >= 2 && beta < INF &&
                      PopCount(board_.BitBoard()) > 10 && !in_check;
    if (allow_null_move) {
      board_.MakeNullMove();
      int value = -PVS(max_depth - 2, -beta, -beta + 1, ply + 1,
                       !allow_null_move, search_stats);
      board_.UnmakeNullMove();
      if (value >= beta) {
        return beta;
      }
    }

    // Is it likely futile to search this position as we may not improve alpha?
    if (!in_check && max_depth == 1) {
      const int eval_score = StaticEval(board_);
      if (eval_score + 450 <= alpha) {
        return Evaluate<variant>(board_, egtb_, alpha, beta);
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
      OrderMoves<variant>(board_, move_array, &pref_moves, history_);

  Move best_move;
  NodeType node_type = NodeType::FAIL_LOW_NODE;
  int b = beta;
  int score = -INF;

  Move searched_quiets[256];
  int num_searched_quiets = 0;

  for (size_t index = 0; index < move_info_array.size; ++index) {
    const MoveInfo& move_info = move_info_array.moves[index];
    const Move move = move_info.move;

    const bool is_capture = board_.PieceAt(move.to_index()) != NULLPIECE;

    board_.MakeMove(move);

    int value = -INF;

    if constexpr (!IsAntichessLike(variant)) {
      if (!in_check && (max_depth == 1 || max_depth == 2) &&
          !move.is_promotion() && move_info.type == MoveType::QUIET &&
          !attacks::InCheck(board_, board_.SideToMove())) {
        const int eval_score = -StaticEval(board_);
        if (eval_score + max_depth * 120 <= alpha) {
          board_.UnmakeLastMove();
          if (!is_capture) {
            searched_quiets[num_searched_quiets++] = move;
          }
          continue;
        }
      }
    }

    int lmr_depth_reduction = LMRDepthReduction<variant>(max_depth, index, move_info);

    // Apply late move reduction if applicable.
    if (lmr_depth_reduction > 0) {
      value =
          -PVS(max_depth - lmr_depth_reduction, -alpha - 1, -alpha, ply + 1, true, search_stats);
    }

    // If LMR was not triggered or LMR search failed high, proceed with normal
    // search.
    if (lmr_depth_reduction == 0 || value > alpha) {
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
              (IsAntichessLike(variant) || !is_capture)) {
            killers_[ply][1] = killers_[ply][0];
            killers_[ply][0] = move;
          }

          if (!is_capture) {
            UpdateHistory(move, max_depth, true);
            for (int k = 0; k < num_searched_quiets; ++k) {
              UpdateHistory(searched_quiets[k], max_depth, false);
            }
          }
          break;
        }
      }
    }
    b = alpha + 1;
    if (!is_capture) {
      searched_quiets[num_searched_quiets++] = move;
    }
  }

  if (!(timer_ && timer_->Lapsed())) {
    transpos_.Put(score, node_type, max_depth, zkey, best_move);
  }
  return score;
}

template class PVSearch<Variant::STANDARD>;
template class PVSearch<Variant::ANTICHESS>;
template class PVSearch<Variant::SUICIDE>;
