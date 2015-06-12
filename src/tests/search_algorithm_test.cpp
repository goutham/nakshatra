#include "board.h"
#include "common.h"
#include "eval.h"
#include "eval_suicide.h"
#include "movegen.h"
#include "search_algorithm.h"
#include "stats.h"
#include "transpos.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

class SearchAlgorithmTest : public testing::Test {
 public:
  SearchAlgorithmTest() {
  }
};

TEST_F(SearchAlgorithmTest, NegaScout) {
  // This position can be won by white at depth 7.
  const std::string board_str = "8/R7/8/8/8/8/8/7k w - -";
  Board board(Variant::SUICIDE, board_str);

  std::unique_ptr<MoveGenerator> movegen(new MoveGeneratorSuicide(board));
  std::unique_ptr<Evaluator> eval(new EvalSuicide(&board,
                                                  movegen.get(),
                                                  nullptr));

  TranspositionTable transpos(1U << 20);  // 1 MB
  SearchAlgorithm search_algorithm(&board, movegen.get(), eval.get(),
                                   nullptr, &transpos, nullptr);
  // Not a win up to depth 6.
  for (int depth = 1; depth <= 6; ++depth) {
    SearchStats search_stats;
    int score = search_algorithm.NegaScout(depth, -INF, INF,
                                           &search_stats);
    std::cout << "depth: " << depth << ", score: " << score << std::endl;
    EXPECT_LT(score, WIN);
    EXPECT_GT(score, -WIN);
  }
  // White wins at depth 7.
  SearchStats search_stats;
  EXPECT_EQ(WIN, search_algorithm.NegaScout(7, -INF, INF,
                                            &search_stats));
}
