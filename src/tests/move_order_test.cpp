#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"

#include <gtest/gtest.h>

TEST(StandardMoveOrderer, Order) {
  Board board(Variant::STANDARD, "2k5/8/3r2n1/2P5/8/6Q1/2B5/1K6 w - -");
  MoveArray move_array;
  GenerateMoves<Variant::STANDARD>(&board, &move_array);
  MoveInfoArray move_info_array;
  OrderMoves<Variant::STANDARD>(&board, move_array, nullptr, &move_info_array);
  const char* good_captures_order[] = {
      "g3d6",
      "c5d6",
      "c2g6",
  };
  // Gaining captures will be at the beginning.
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(good_captures_order[i], move_info_array.moves[i].move.str());
  }
  // Losing capture will be at the end.
  EXPECT_EQ("g3g6", move_info_array.moves[move_info_array.size - 1].move.str());
}
