#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"

#include <gtest/gtest.h>

TEST(StandardMoveOrderer, Order) {
  Board board(Variant::STANDARD, "2k5/8/3r2n1/2P5/8/6Q1/2B5/1K6 w - -");
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);
  const MoveInfoArray move_info_array =
      OrderMoves<Variant::STANDARD>(board, move_array, nullptr);
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

TEST(StandardMoveOrderer, ComprehensiveOrder) {
  // Setup a position with various move types:
  // White: K e1, Q d1, N b1, P a7 (prom), P e4, P g2.
  // Black: k e8, q f3, p c6, p d5.
  // FEN: "4k3/P7/2p5/3p4/4P3/5q2/6P1/1N1QK3 w - - 0 1"
  Board board(Variant::STANDARD, "4k3/P7/2p5/3p4/4P3/5q2/6P1/1N1QK3 w - - 0 1");
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);

  // Define interesting moves
  const Move tt_move("b1c3");
  const Move prom_move("a7a8q");
  const Move good_cap_high("g2f3"); // PxQ
  const Move good_cap_low("e4d5");  // PxP (trade)
  const Move killer1("b1a3");
  const Move killer2("e1d2");
  const Move quiet_high_hist("d1e2");
  const Move quiet_low_hist("d1c2");
  const Move bad_cap("d1d5");       // QxP (bad trade)

  // Setup PrefMoves
  PrefMoves pref_moves;
  pref_moves.tt_move = tt_move;
  pref_moves.killer1 = killer1;
  pref_moves.killer2 = killer2;

  // Setup History
  // Dimensions: 12x64.
  int history[12][64] = {{0}};
  // Set history for d1e2 (Queen move).
  // Queen = 2.
  history[QUEEN][INDX("e2")] = 10000;
  history[QUEEN][INDX("c2")] = 100;

  // Order moves
  MoveInfoArray result = OrderMoves<Variant::STANDARD>(board, move_array, &pref_moves, history);

  // Helper to find index in sorted result
  auto get_index = [&](const Move& m) -> int {
    for (size_t i = 0; i < result.size; ++i) {
      if (result.moves[i].move == m) return i;
    }
    return -1;
  };

  int idx_tt = get_index(tt_move);
  int idx_prom = get_index(prom_move);
  int idx_good_high = get_index(good_cap_high);
  int idx_good_low = get_index(good_cap_low);
  int idx_killer1 = get_index(killer1);
  int idx_killer2 = get_index(killer2);
  int idx_quiet_high = get_index(quiet_high_hist);
  int idx_quiet_low = get_index(quiet_low_hist);
  int idx_bad = get_index(bad_cap);

  // Ensure all moves are present
  EXPECT_NE(idx_tt, -1) << "TT move not found";
  EXPECT_NE(idx_prom, -1) << "Promotion move not found";
  EXPECT_NE(idx_good_high, -1) << "High good capture not found";
  EXPECT_NE(idx_good_low, -1) << "Low good capture not found";
  EXPECT_NE(idx_killer1, -1) << "Killer1 not found";
  EXPECT_NE(idx_killer2, -1) << "Killer2 not found";
  EXPECT_NE(idx_quiet_high, -1) << "Quiet high hist not found";
  EXPECT_NE(idx_quiet_low, -1) << "Quiet low hist not found";
  EXPECT_NE(idx_bad, -1) << "Bad capture not found";

  // Assertions based on priority:
  // 1. TT
  // 2. Queen Promotion
  // 3. Good Captures (sorted by SEE)
  // 4. Killers
  // 5. Quiet (History)
  // 6. Bad Captures

  EXPECT_LT(idx_tt, idx_prom) << "TT should be before Promotion";
  EXPECT_LT(idx_prom, idx_good_high) << "Promotion should be before Good Capture";
  EXPECT_LT(idx_good_high, idx_killer1) << "Good Capture should be before Killer";
  EXPECT_LT(idx_good_low, idx_killer1) << "Good Capture (low) should be before Killer";
  
  // Between Good Captures
  // g2f3 (PxQ) vs e4d5 (PxP). PxQ is huge gain.
  EXPECT_LT(idx_good_high, idx_good_low) << "High value capture should be before low value";

  EXPECT_LT(idx_killer1, idx_killer2) << "Killer1 should be before Killer2";
  EXPECT_LT(idx_killer2, idx_quiet_high) << "Killer2 should be before Quiet moves";
  
  EXPECT_LT(idx_quiet_high, idx_quiet_low) << "High history quiet should be before low history";
  
  EXPECT_LT(idx_quiet_low, idx_bad) << "Quiet move should be before Bad Capture";
}