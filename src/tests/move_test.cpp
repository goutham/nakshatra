#include "move.h"
#include "move.h"
#include <gtest/gtest.h>

TEST(MoveTest, VerifyMoves) {
  EXPECT_EQ(2, sizeof(Move));
  Move move1("e2e4");
  EXPECT_EQ(1, ROW(move1.from_index()));
  EXPECT_EQ(4, COL(move1.from_index()));
  EXPECT_EQ(3, ROW(move1.to_index()));
  EXPECT_EQ(4, COL(move1.to_index()));
  EXPECT_EQ(NULLPIECE, move1.promoted_piece());
  EXPECT_TRUE(move1.is_valid());

  Move move2("h7g8q");
  EXPECT_EQ(6, ROW(move2.from_index()));
  EXPECT_EQ(7, COL(move2.from_index()));
  EXPECT_EQ(7, ROW(move2.to_index()));
  EXPECT_EQ(6, COL(move2.to_index()));
  EXPECT_EQ(-QUEEN, move2.promoted_piece());
  EXPECT_TRUE(move2.is_valid());

  Move move3("f2f1B");
  EXPECT_EQ(1, ROW(move3.from_index()));
  EXPECT_EQ(5, COL(move3.from_index()));
  EXPECT_EQ(0, ROW(move3.to_index()));
  EXPECT_EQ(5, COL(move3.to_index()));
  EXPECT_EQ(BISHOP, move3.promoted_piece());
  EXPECT_TRUE(move3.is_valid());

  Move move4;
  EXPECT_FALSE(move4.is_valid());
}
