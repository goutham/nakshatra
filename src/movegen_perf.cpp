#include "board.h"
#include "common.h"
#include "move_array.h"
#include "movegen.h"
#include "stopwatch.h"

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>

template <Variant variant>
int64_t Perft(Board* board, unsigned int depth) {
  MoveArray move_array;
  GenerateMoves<variant>(board, &move_array);

  // Bulk counting at depth 1.
  if (depth == 1) {
    return move_array.size();
  }

  int64_t nodes = 0;
  for (unsigned i = 0; i < move_array.size(); ++i) {
    board->MakeMove(move_array.get(i));
    nodes += Perft<variant>(board, depth - 1);
    board->UnmakeLastMove();
  }
  return nodes;
}

int main(int argc, char** argv) {
  assert(argc == 3);

  unsigned int depth = 0;
  Variant variant;
  std::function<int64_t(Board * board, int depth)> perft_fn;
  if (argv[1][0] == 's' || argv[1][0] == 'S') {
    variant = Variant::ANTICHESS;
    perft_fn = Perft<Variant::ANTICHESS>;
  } else {
    variant = Variant::STANDARD;
    perft_fn = Perft<Variant::STANDARD>;
  }
  Board board(variant);
  depth = atoi(argv[2]);

  StopWatch stop_watch;
  stop_watch.Start();
  int64_t nodes = perft_fn(&board, depth);
  stop_watch.Stop();
  const double elapsed_secs = stop_watch.ElapsedTime() / 100.0;

  printf(
      "+--------+------------------+-----------------+--------------------+\n");
  printf(
      "| Depth  | Elapsed time (s) |    Num Nodes    |   Num Nodes / sec  |\n");
  printf(
      "+--------+------------------+-----------------+--------------------+\n");
  printf("|%6d  | %16.3f | %12" PRId64 "    | %17.3f  |\n", depth, elapsed_secs,
         nodes, static_cast<double>(nodes) / elapsed_secs);
  printf(
      "+--------+------------------+-----------------+--------------------+\n");

  return 0;
}
