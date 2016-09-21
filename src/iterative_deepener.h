#ifndef ITERATIVE_DEEPENER_H
#define ITERATIVE_DEEPENER_H

#include "move.h"
#include "move_array.h"
#include "stats.h"

#include <vector>

class Board;
class Extensions;
class MoveGenerator;
class SearchAlgorithm;
class Timer;
class TranspositionTable;

// Iterative deepening search parameters.
struct IDSParams {
  bool thinking_output = false;
  unsigned search_depth = MAX_DEPTH;
  MoveArray pruned_ordered_moves;
};

class IterativeDeepener {
 public:
  IterativeDeepener(Board* board,
                    MoveGenerator* movegen,
                    SearchAlgorithm* search_algorithm,
                    Timer* timer,
                    TranspositionTable* transpos,
                    Extensions* extensions)
      : board_(board),
        movegen_(movegen),
        search_algorithm_(search_algorithm),
        timer_(timer),
        transpos_(transpos),
        extensions_(extensions) {}

  void Search(const IDSParams& ids_params,
              Move* best_move,
              int* best_move_score,
              SearchStats* id_search_stats);

 private:
  void FindBestMove(int max_depth);

  // Returns principal variation as a string of moves.
  std::string PV(const Move& root_move);

  void ClearState();

  Board* board_;
  MoveGenerator* movegen_;
  SearchAlgorithm* search_algorithm_;
  Timer* timer_;
  TranspositionTable* transpos_;
  Extensions* extensions_;

  // Maintains list of moves at the root node.
  MoveArray root_move_array_;

  // Stat associated with each iteration of the iterative deepening search
  // is stored in the corresponding IterationStat object and pushed in the
  // iteration_stats_ vector.
  struct IterationStat {
    IterationStat() : depth(0), score(0), root_moves_covered(0) {}

    // Depth of search for current iteration.
    int depth;

    // Best move found in this iteration.
    Move best_move;

    // Score for the best move.
    int score;

    // Number of root moves completely searched before timer
    // expired at current depth.
    int root_moves_covered;

    // Stats for searching to this depth.
    SearchStats search_stats;
  };

  std::vector<IterationStat> iteration_stats_;
};

#endif
