#include "move_order.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "pst.h"
#include "see.h"

template <>
void OrderMoves<Variant::STANDARD>(Board* board, const MoveArray& move_array,
                                   const PrefMoves* pref_moves,
                                   MoveInfoArray* move_info_array) {
  const size_t num_moves = move_array.size();
  move_info_array->size = num_moves;
  for (size_t i = 0; i < num_moves; ++i) {
    const Move move = move_array.get(i);
    if (pref_moves && pref_moves->tt_move == move) {
      move_info_array->moves[i] = {move, MoveType::TT, 0};
    } else if (board->PieceAt(move.to_index()) != NULLPIECE) {
      const int see_val = SEE(move, *board);
      const auto type = (see_val >= 0) ? MoveType::SEE_GOOD_CAPTURE
                                       : MoveType::SEE_BAD_CAPTURE;
      move_info_array->moves[i] = {move, type, see_val};
    } else if (pref_moves && pref_moves->killer1 == move) {
      move_info_array->moves[i] = {move, MoveType::KILLER, 1};
    } else if (pref_moves && pref_moves->killer2 == move) {
      move_info_array->moves[i] = {move, MoveType::KILLER, 0};
    } else {
      const int from_sq = move.from_index();
      const int to_sq = move.to_index();
      const Side side = board->SideToMove();
      const Piece piece = board->PieceAt(from_sq);
      move_info_array->moves[i] = {move, MoveType::QUIET,
                                   standard::PSTVal(side, piece, to_sq) -
                                       standard::PSTVal(side, piece, from_sq)};
    }
  }
  move_info_array->Sort();
}

template <>
void OrderMoves<Variant::ANTICHESS>(Board* board, const MoveArray& move_array,
                                    const PrefMoves* pref_moves,
                                    MoveInfoArray* move_info_array) {
  const size_t num_moves = move_array.size();
  move_info_array->size = num_moves;
  for (size_t i = 0; i < num_moves; ++i) {
    const Move move = move_array.get(i);
    if (pref_moves && pref_moves->tt_move == move) {
      move_info_array->moves[i] = {move, MoveType::TT, 0};
    } else if (pref_moves && pref_moves->killer1 == move) {
      move_info_array->moves[i] = {move, MoveType::KILLER, 1};
    } else if (pref_moves && pref_moves->killer2 == move) {
      move_info_array->moves[i] = {move, MoveType::KILLER, 0};
    } else {
      board->MakeMove(move);
      const int opp_moves = CountMoves<Variant::ANTICHESS>(board);
      move_info_array->moves[i] = {move, MoveType::UNCATEGORIZED, -opp_moves};
      board->UnmakeLastMove();
    }
  }
  move_info_array->Sort();
}

template <Variant variant>
void OrderMovesByEvalScore(Board* board, const MoveArray& move_array,
                           const PrefMoves* pref_moves,
                           MoveInfoArray* move_info_array) {
  assert(pref_moves == nullptr);
  const size_t num_moves = move_array.size();
  move_info_array->size = num_moves;
  for (size_t i = 0; i < num_moves; ++i) {
    const Move move = move_array.get(i);
    board->MakeMove(move);
    move_info_array->moves[i] = {move, MoveType::UNCATEGORIZED,
                                 -Evaluate<variant>(board, -INF, +INF)};
    board->UnmakeLastMove();
  }
  move_info_array->Sort();
}

template void OrderMovesByEvalScore<Variant::STANDARD>(
    Board* board, const MoveArray& move_array, const PrefMoves* pref_moves,
    MoveInfoArray* move_info_array);
template void OrderMovesByEvalScore<Variant::ANTICHESS>(
    Board* board, const MoveArray& move_array, const PrefMoves* pref_moves,
    MoveInfoArray* move_info_array);
