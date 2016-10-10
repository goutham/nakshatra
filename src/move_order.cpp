#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

void MobilityOrderer::Order(MoveArray* move_array) {
  struct OpponentMoves {
    unsigned opp_moves;
    unsigned index;
  };
  OpponentMoves num_opponent_moves[256];
  for (unsigned i = 0; i < move_array->size(); ++i) {
    board_->MakeMove(move_array->get(i));
    const unsigned num_moves = movegen_->CountMoves();
    num_opponent_moves[i] = {num_moves, i};
    board_->UnmakeLastMove();
  }
  std::sort(num_opponent_moves, num_opponent_moves + move_array->size(),
      [](const OpponentMoves& a, const OpponentMoves& b) {
        return a.opp_moves < b.opp_moves;
      });
  MoveArray new_move_array;
  for (unsigned i = 0; i < move_array->size(); ++i) {
    new_move_array.Add(move_array->get(num_opponent_moves[i].index));
  }
  memcpy(move_array, &new_move_array, sizeof(MoveArray));
}

void CapturesFirstOrderer::Order(MoveArray* move_array) {
  const size_t num_moves = move_array->size();
  struct CapturesDiff {
    Move move;
    int diff;
  };
  auto diff = [&](const Move& move) -> int {
    return abs(PieceType(board_->PieceAt(move.from_index()))) -
           abs(PieceType(board_->PieceAt(move.to_index())));
  };
  CapturesDiff captures_diff[256];
  int j = 0;
  MoveArray non_captures;
  for (size_t i = 0; i < num_moves; ++i) {
    const Move& move = move_array->get(i);
    if (board_->PieceAt(move.to_index()) != NULLPIECE) {
      captures_diff[j] = {move, diff(move)};
      ++j;
    } else {
      non_captures.Add(move);
    }
  }
  std::sort(captures_diff, captures_diff + j,
       [](const CapturesDiff & a, const CapturesDiff & b)
           ->bool { return a.diff < b.diff; });
  move_array->clear();
  for (int i = 0; i < j; ++i) {
    move_array->Add(captures_diff[i].move);
  }
  for (size_t i = 0; i < non_captures.size(); ++i) {
    move_array->Add(non_captures.get(i));
  }
  assert(num_moves == move_array->size());
}
