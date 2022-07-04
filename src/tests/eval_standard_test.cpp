#include "board.h"
#include "common.h"
#include "eval_standard.h"
#include "movegen.h"
#include <iostream>

#include <gtest/gtest.h>

TEST(EvalStandardTest, Result) {
  Board board(Variant::STANDARD, "4k3/8/1npp3p/p7/7p/K1r5/5r1P/8 w - -");
  EvalStandard eval(&board);
  EXPECT_EQ(-WIN, eval.Result());
}
