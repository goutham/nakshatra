#ifndef ITERATIVE_DEEPENER_H
#define ITERATIVE_DEEPENER_H

#include "common.h"
#include "move.h"
#include "move_array.h"
#include "stats.h"

class Board;
class Timer;
class TranspositionTable;

// Iterative deepening search parameters.
struct IDSParams {
  bool thinking_output = false;
  unsigned search_depth = MAX_DEPTH;
  MoveArray pruned_ordered_moves;
};

void IDSearch(const Variant variant, const IDSParams& ids_params, Board* board,
              Timer* timer, TranspositionTable* transpos, Move* best_move,
              int* best_move_score, SearchStats* id_search_stats);

#endif
