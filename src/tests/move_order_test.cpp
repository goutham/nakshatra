#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"

#include <gtest/gtest.h>

TEST(CapturesFirstOrdererTest, CapturesFirst) {
  Board board(Variant::STANDARD, "2k5/8/3r2n1/2P5/8/6Q1/2B5/1K6 w - -");
  MoveGeneratorStandard movegen(&board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  StandardMoveOrderer orderer(&board);
  orderer.Order(&move_array);
  const char* captures_order[] = {"g3d6", "c5d6", "c2g6",};
  // Gaining captures will be at the beginning.
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(captures_order[i], move_array.get(i).str());
  }
  // Losing capture will be at the end.
  EXPECT_EQ("g3g6", move_array.get(move_array.size() - 1).str());
}