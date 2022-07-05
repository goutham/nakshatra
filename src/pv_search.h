#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"

#include <atomic>

class Board;
class Timer;
class TranspositionTable;
struct SearchStats;

class PVSearch {
public:
  PVSearch(const Variant variant, Board* board, Timer* timer,
           TranspositionTable* transpos,
           std::atomic<bool>* abort_flag = nullptr)
      : variant_(variant), board_(board), timer_(timer), transpos_(transpos),
        abort_flag_(abort_flag) {}

  int Search(int max_depth, int alpha, int beta, SearchStats* search_stats);

private:
  int PVS(int max_depth, int alpha, int beta, int ply, bool allow_null_move,
          SearchStats* search_stats);

  const Variant variant_;
  Board* board_;
  Timer* timer_;
  TranspositionTable* transpos_;
  std::atomic<bool>* abort_flag_ = nullptr;
  Move killers_[MAX_DEPTH][2];
};

#endif
