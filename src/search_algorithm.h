#ifndef SEARCH_H
#define SEARCH_H

class Evaluator;
class Extensions;
class MoveGenerator;
class Timer;
class TranspositionTable;
struct SearchStats;

class SearchAlgorithm {
 public:
  SearchAlgorithm(Board* board,
                  MoveGenerator* movegen,
                  Evaluator* evaluator,
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
  MoveGenerator* movegen_;
  Timer* timer_;
  Evaluator* evaluator_;
  TranspositionTable* transpos_;
  Extensions* extensions_;
};

#endif
