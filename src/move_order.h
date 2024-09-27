#ifndef MOVE_ORDER_H
#define MOVE_ORDER_H

#include "board.h"
#include "common.h"
#include "egtb.h"
#include "move.h"
#include "move_array.h"

#include <algorithm>
#include <vector>

enum class MoveType {
  TT,
  SEE_GOOD_CAPTURE,
  KILLER,
  QUIET,
  SEE_BAD_CAPTURE,
  UNCATEGORIZED
};

struct MoveInfo {
  Move move;
  MoveType type;
  int score;
};

struct MoveInfoArray {
  MoveInfo moves[256];
  size_t size;

  void Sort() {
    std::sort(
        moves, moves + size, [](const MoveInfo& a, const MoveInfo& b) -> bool {
          return a.type < b.type || (a.type == b.type && a.score > b.score);
        });
  }
};

// Container for any preferred moves supplied by caller that need to be treated
// with special care.
struct PrefMoves {
  Move tt_move = Move();
  Move killer1 = Move();
  Move killer2 = Move();
};

template <Variant variant>
  requires(IsStandard(variant))
MoveInfoArray OrderMoves(Board& board, const MoveArray& move_array,
                         const PrefMoves* pref_moves);

template <Variant variant>
  requires(IsAntichessLike(variant))
MoveInfoArray OrderMoves(Board& board, const MoveArray& move_array,
                         const PrefMoves* pref_moves);

template <Variant variant>
MoveInfoArray OrderMovesByEvalScore(Board& board, EGTB* egtb,
                                    const MoveArray& move_array,
                                    const PrefMoves* pref_moves);

#endif
