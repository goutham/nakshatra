#include "move_order.h"
#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "see.h"

#include <algorithm>

struct MoveInfo {
  Move move;
  int score;
};

void AntichessMoveOrderer::Order(MoveArray* move_array,
                                 const PrefMoves* pref_moves) {
  const size_t num_moves = move_array->size();
  MoveInfo move_infos[256];
  for (size_t i = 0; i < num_moves; ++i) {
    const Move move = move_array->get(i);
    if (pref_moves && pref_moves->tt_move == move) {
      move_infos[i] = {move, INF};
    } else if (pref_moves && pref_moves->killer1 == move) {
      move_infos[i] = {move, INF - 1};
    } else if (pref_moves && pref_moves->killer2 == move) {
      move_infos[i] = {move, INF - 2};
    } else {
      board_->MakeMove(move);
      const int opp_moves = movegen_->CountMoves();
      move_infos[i] = {move, -opp_moves};
      board_->UnmakeLastMove();
    }
  }
  std::sort(move_infos, move_infos + num_moves,
            [](const MoveInfo& a, const MoveInfo& b) -> bool {
              return a.score > b.score;
            });
  move_array->clear();
  for (size_t i = 0; i < num_moves; ++i) {
    move_array->Add(move_infos[i].move);
  }
}

void StandardMoveOrderer::Order(MoveArray* move_array,
                                const PrefMoves* pref_moves) {
  const size_t num_moves = move_array->size();
  MoveInfo move_infos[256];
  for (size_t i = 0; i < num_moves; ++i) {
    const Move move = move_array->get(i);
    if (pref_moves && pref_moves->tt_move == move) {
      move_infos[i] = {move, INF};
    } else if (board_->PieceAt(move.to_index()) != NULLPIECE) {
      move_infos[i] = {move, SEE(move, *board_)};
    } else if (pref_moves && pref_moves->killer1 == move) {
      move_infos[i] = {move, -1}; // lower prio than good captures
    } else if (pref_moves && pref_moves->killer2 == move) {
      move_infos[i] = {move, -2};
    } else {
      move_infos[i] = {move, -3};
    }
  }
  std::sort(move_infos, move_infos + num_moves,
            [](const MoveInfo& a, const MoveInfo& b) -> bool {
              return a.score > b.score;
            });
  move_array->clear();
  for (size_t i = 0; i < num_moves; ++i) {
    move_array->Add(move_infos[i].move);
  }
}
