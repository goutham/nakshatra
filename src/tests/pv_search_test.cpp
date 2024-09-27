#include "board.h"
#include "common.h"
#include "eval.h"
#include "move_order.h"
#include "movegen.h"
#include "pv_search.h"
#include "stats.h"
#include "transpos.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

class PVSearchTest : public testing::Test {
public:
  PVSearchTest() {}
};

TEST_F(PVSearchTest, Search) {
  // This position can be won by white at depth 7.
  const std::string board_str = "8/R7/8/8/8/8/8/7k w - -";
  Board board(Variant::ANTICHESS, board_str);
  TranspositionTable transpos(1U << 20); // 1 MB
  PVSearch<Variant::ANTICHESS> pv_search(board, nullptr, transpos, nullptr);
  // Not a win up to depth 6.
  for (int depth = 1; depth <= 6; ++depth) {
    SearchStats search_stats;
    int score = pv_search.Search(depth, -INF, INF, search_stats);
    std::cout << "depth: " << depth << ", score: " << score << std::endl;
    EXPECT_LT(score, WIN);
    EXPECT_GT(score, -WIN);
  }
  // White wins at depth 7.
  SearchStats search_stats;
  EXPECT_EQ(WIN, pv_search.Search(7, -INF, INF, search_stats));
}

TEST_F(PVSearchTest, Repetition) {
  Board board(Variant::STANDARD, "k7/n7/8/8/8/7B/7N/7K w - -");
  TranspositionTable tt(256);
  PVSearch<Variant::STANDARD> pv_search(board, nullptr, tt, nullptr);
  SearchStats stats;
  EXPECT_GT(pv_search.Search(1, -INF, INF, stats), 0);
  board.MakeMove(Move("h2f3"));
  board.MakeMove(Move("a7c6"));
  EXPECT_GT(pv_search.Search(1, -INF, INF, stats), 0);
  board.MakeMove(Move("f3e1"));
  board.MakeMove(Move("c6a5"));
  EXPECT_GT(pv_search.Search(1, -INF, INF, stats), 0);
  board.MakeMove(Move("e1f3"));
  board.MakeMove(Move("a5c6"));
  EXPECT_EQ(DRAW, pv_search.Search(1, -INF, INF, stats));
  board.MakeMove(Move("f3h2"));
  board.MakeMove(Move("c6a7"));
  EXPECT_EQ(8, board.HalfMoveClock());
  EXPECT_EQ(8, board.HalfMoves());
  EXPECT_EQ(DRAW, pv_search.Search(1, -INF, INF, stats));
}
