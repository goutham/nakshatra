#include "board.h"
#include "common.h"
#include "move.h"
#include "transpos.h"
#include "zobrist.h"
#include <gtest/gtest.h>

TEST(TransposTest, VerifyEntries) {
  Board board(Variant::ANTICHESS,
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - -");
  TranspositionTable t(1024);
  U64 zkey = board.ZobristKey();
  EXPECT_EQ(nullptr, t.Get(zkey));

  Move m("e2e3");
  t.Put(1000, FAIL_HIGH_NODE, 10, zkey, m);
  EXPECT_EQ(1000, t.Get(zkey)->score);
  EXPECT_EQ(FAIL_HIGH_NODE, t.Get(zkey)->node_type());
  EXPECT_EQ(10, t.Get(zkey)->depth);
  EXPECT_EQ(zkey, t.Get(zkey)->zkey);
  EXPECT_EQ(m.encoded_move(), t.Get(zkey)->best_move.encoded_move());
}
