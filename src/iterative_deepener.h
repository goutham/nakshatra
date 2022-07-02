#ifndef ITERATIVE_DEEPENER_H
#define ITERATIVE_DEEPENER_H

#include "move.h"
#include "move_array.h"
#include "move_order.h"
#include "stats.h"

#include <map>
#include <utility>
#include <vector>

class Board;
class EGTB;
class MoveGenerator;
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
  IterativeDeepener(const Variant variant, Board* board, MoveGenerator* movegen,
                    Timer* timer, TranspositionTable* transpos,
                    MoveOrderer* move_orderer, EGTB* egtb)
      : variant_(variant), board_(board), movegen_(movegen), timer_(timer),
        transpos_(transpos), move_orderer_(move_orderer), egtb_(egtb) {}

  void Search(const IDSParams& ids_params, Move* best_move,
              int* best_move_score, SearchStats* id_search_stats);

private:
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
    std::vector<std::pair<Move, SearchStats>> move_stats;

    void MergeStats(const IterationStat& istat) {
      std::map<Move, SearchStats> stats_by_move;
      for (const auto& item : istat.move_stats) {
        stats_by_move[item.first] = item.second;
      }
      for (auto& item : move_stats) {
        auto iter = stats_by_move.find(item.first);
        if (iter != stats_by_move.end()) {
          item.second.nodes_searched += iter->second.nodes_searched;
        }
      }
    }
  };

  IterationStat FindBestMove(int max_depth);

  // Returns principal variation as a string of moves.
  std::string PV(const Move& root_move);

  const Variant variant_;
  Board* board_;
  MoveGenerator* movegen_;
  Timer* timer_;
  TranspositionTable* transpos_;
  MoveOrderer* move_orderer_;
  EGTB* egtb_;

  // Maintains list of moves at the root node.
  MoveArray root_move_array_;

  std::vector<IterationStat> iteration_stats_;
};

#endif
