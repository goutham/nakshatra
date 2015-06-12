#include "board.h"
#include "common.h"
#include "pawns.h"

#include <gtest/gtest.h>

TEST(PawnsTest, DoubledPawns) {
  Board board(Variant::SUICIDE, "8/p3p3/pP1p4/8/1PPp4/8/8/8 w - -");
  EXPECT_EQ(1, PopCount(pawns::DoubledPawns(board, Side::WHITE)));
  EXPECT_EQ(2, PopCount(pawns::DoubledPawns(board, Side::BLACK)));
}
