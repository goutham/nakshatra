#ifndef SEARCH_H
#define SEARCH_H

class Extensions;
class Timer;

namespace eval {
class Evaluator;
}

namespace movegen {
class MoveGenerator;
}

namespace search {

struct SearchStats;
class TranspositionTable;

class SearchAlgorithm {
 public:
  SearchAlgorithm(Board* board,
                  movegen::MoveGenerator* movegen,
                  eval::Evaluator* evaluator,
                  Timer* timer,
                  TranspositionTable* transpos,
                  Extensions* extensions)
      : board_(board),
        movegen_(movegen),
        timer_(timer),
        evaluator_(evaluator),
        transpos_(transpos),
        extensions_(extensions) {}

  int NegaScout(int max_depth,
                int alpha,
                int beta,
                SearchStats* search_stats);

 private:
  Board* board_;
  movegen::MoveGenerator* movegen_;
  eval::Evaluator* evaluator_;
  Timer* timer_;
  TranspositionTable* transpos_;
  Extensions* extensions_;
};

}  // namespace search

#endif
