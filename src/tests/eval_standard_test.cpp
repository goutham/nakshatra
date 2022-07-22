#include "board.h"
#include "common.h"
#include "eval.h"
#include "movegen.h"
#include <iostream>

#include <gtest/gtest.h>

TEST(EvalStandardTest, Result) {
  Board board(Variant::STANDARD, "4k3/8/1npp3p/p7/7p/K1r5/5r1P/8 w - -");
  EXPECT_EQ(-WIN, EvalResult<Variant::STANDARD>(&board));
}

TEST(EvalStandardTest, EvaluateCheckmate) {
  Board board(Variant::STANDARD, "4k3/8/1npp3p/p7/7p/K1r5/5r1P/8 w - -");
  EXPECT_EQ(-WIN, Evaluate<Variant::STANDARD>(&board, -INF, INF));
}

TEST(EvalStandardTest, EvaluateStalemate) {
  Board board(Variant::STANDARD, "1r1b4/8/8/8/8/4k3/4p3/4K3 w - - ");
  EXPECT_EQ(DRAW, Evaluate<Variant::STANDARD>(&board, -INF, INF));
}
