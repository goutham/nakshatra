#ifndef ITERATIVE_DEEPENER_H
#define ITERATIVE_DEEPENER_H

#include "board.h"
#include "common.h"
#include "egtb.h"
#include "move.h"
#include "move_array.h"
#include "stats.h"
#include "timer.h"
#include "transpos.h"

// Iterative deepening search parameters.
struct IDSParams {
  bool thinking_output = false;
  unsigned search_depth = MAX_DEPTH;
  MoveArray pruned_ordered_moves;
};

template <Variant variant>
void IDSearch(const IDSParams& ids_params, Board* board, Timer* timer,
              TranspositionTable* transpos, EGTB* egtb, Move* best_move,
              int* best_move_score, SearchStats* id_search_stats);

#endif
