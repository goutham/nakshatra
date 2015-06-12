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
#include <iostream>

int64_t Perft(MoveGenerator* movegen, Board* board, unsigned int depth) {
  if (depth == 0) {
    return 1;
  }
  MoveArray move_array;
  movegen->GenerateMoves(&move_array);
  int64_t nodes = 0;
  for (unsigned i = 0; i < move_array.size(); ++i) {
    board->MakeMove(move_array.get(i));
    nodes += Perft(movegen, board, depth - 1);
    board->UnmakeLastMove();
  }
  return nodes;
}

int main(int argc, char **argv) {
  assert(argc == 3);

  unsigned int depth = 0;
  Board* board = NULL;
  MoveGenerator* movegen = NULL;
  if (argv[1][0] == 's' || argv[1][0] == 'S') {
    board = new Board(SUICIDE);
    movegen = new MoveGeneratorSuicide(*board);
  } else {
    board = new Board(NORMAL);
    movegen = new MoveGeneratorNormal(board);
  }
  depth = atoi(argv[2]);

  StopWatch stop_watch;
  stop_watch.Start();
  int64_t nodes = Perft(movegen, board, depth);
  stop_watch.Stop();
  delete movegen;
  delete board;
  const double elapsed_secs = stop_watch.ElapsedTime() / 100.0;

  printf("+--------+------------------+-----------------+--------------------+\n");
  printf("| Depth  | Elapsed time (s) |    Num Nodes    |   Num Nodes / sec  |\n");
  printf("+--------+------------------+-----------------+--------------------+\n");
  printf("|%6d  | %16.3f | %12" PRId64 "    | %17.3f  |\n", depth, elapsed_secs,
         nodes, static_cast<double>(nodes) / elapsed_secs);
  printf("+--------+------------------+-----------------+--------------------+\n");

  return 0;
}
