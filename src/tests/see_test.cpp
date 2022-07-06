#include "board.h"
#include "common.h"
#include "see.h"

#include <gtest/gtest.h>

TEST(SEETest, SEE) {
  Board board(Variant::STANDARD, "2k5/8/3r2n1/2P5/8/6Q1/2B5/1K6 w - -");
  EXPECT_EQ(-80, SEE(Move("g3g6"), board));
  EXPECT_EQ(500, SEE(Move("c5d6"), board));
  EXPECT_EQ(320, SEE(Move("c2g6"), board));
  EXPECT_EQ(500, SEE(Move("g3d6b"), board));
}

TEST(SEETest, SEEDeepExchange) {
  Board board(Variant::STANDARD,
              "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -");
  EXPECT_EQ(-220, SEE(Move("d3e5"), board));
}

TEST(SEETest, SEEBlockedRook) {
  Board board(Variant::STANDARD, "1q4k1/8/3b4/8/8/3R4/3P4/1K1R4 w - -");
  EXPECT_EQ(-170, SEE(Move("d3d6"), board));
}
