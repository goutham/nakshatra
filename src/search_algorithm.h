#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"

#include <atomic>

class Board;
class Evaluator;
class MoveGenerator;
class MoveOrderer;
class Timer;
class TranspositionTable;
struct SearchStats;

class SearchAlgorithm {
public:
  SearchAlgorithm(const Variant variant, Board* board, Evaluator* evaluator,
                  Timer* timer, TranspositionTable* transpos,
                  MoveOrderer* move_orderer, std::atomic<bool>* abort = nullptr)
      : variant_(variant), board_(board), timer_(timer), evaluator_(evaluator),
        transpos_(transpos), move_orderer_(move_orderer), abort_(abort) {}

  int Search(int max_depth, int alpha, int beta, SearchStats* search_stats);

private:
  int NegaScout(int max_depth, int alpha, int beta, int ply,
                bool allow_null_move, SearchStats* search_stats);

  const Variant variant_;
  Board* board_;
  Timer* timer_;
  Evaluator* evaluator_;
  TranspositionTable* transpos_;
  MoveOrderer* move_orderer_;
  std::atomic<bool>* abort_ = nullptr;
  Move killers_[MAX_DEPTH][2];
};

#endif
