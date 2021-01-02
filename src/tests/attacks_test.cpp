#include "attacks.h"
#include "board.h"
#include "common.h"

#include <gtest/gtest.h>

using namespace std;

TEST(AttacksTest, SquareAttackers_AttackByWhitePawns) {
  const string fen = "6k1/8/3P1P2/4r3/3P4/5P2/8/K7 w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("e5");
  const U64 bb = SetBit("d4");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, PAWN, board.BitBoard(),
                                         board.BitBoard(PAWN)));
}

TEST(AttacksTest, SquareAttackers_AttackByBlackPawns) {
  const string fen = "1k5k/8/8/1pppp3/3Q4/2p5/8/5K2 b - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("d4");
  const U64 bb = SetBit("c5") | SetBit("e5");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, -PAWN, board.BitBoard(),
                                         board.BitBoard(-PAWN)));
}

TEST(AttacksTest, SquareAttackers_Queen) {
  const string fen = "k7/8/1q1qq2q/1q6/q7/q3R3/6K1/2q5 w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("e3");
  const U64 bb =
      SetBit("a3") | SetBit("c1") | SetBit("e6") | SetBit("b6") | SetBit("h6");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, -QUEEN, board.BitBoard(),
                                         board.BitBoard(-QUEEN)));
}

TEST(AttacksTest, SquareAttackers_Rook) {
  const string fen = "6k1/4R3/3B4/RR2pQ2/R7/7R/4R3/3K4 w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("e5");
  const U64 bb = SetBit("e2") | SetBit("b5") | SetBit("e7");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, ROOK, board.BitBoard(),
                                         board.BitBoard(ROOK)));
}

TEST(AttacksTest, SquareAttackers_Bishop) {
  const string fen = "B5k1/3Q4/Q1n5/6B1/B3B3/5Q2/6B1/1K5B w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("c6");
  const U64 bb = SetBit("a8") | SetBit("a4") | SetBit("e4");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, BISHOP, board.BitBoard(),
                                         board.BitBoard(BISHOP)));
}

TEST(AttacksTest, SquareAttackers_Knight) {
  const string fen = "4k3/8/8/n1n1n3/3n4/1Q6/3N4/n2K4 w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("b3");
  const U64 bb = SetBit("a1") | SetBit("a5") | SetBit("c5") | SetBit("d4");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, -KNIGHT, board.BitBoard(),
                                         board.BitBoard(-KNIGHT)));
}

TEST(AttacksTest, SquareAttackers_King) {
  const string fen = "3k4/8/8/8/8/2rnp3/2QKN3/3b4 w - -";
  Board board(Variant::STANDARD, fen);
  const int sq = INDX("c3");
  const U64 bb = SetBit("d2");
  EXPECT_EQ(bb, attacks::SquareAttackers(sq, KING, board.BitBoard(),
                                         board.BitBoard(KING)));
}

TEST(AttacksTest, InCheck) {
  {
    Board board(Variant::STANDARD, "5k2/8/6q1/8/8/3K4/8/8 w - -");
    EXPECT_TRUE(attacks::InCheck(board, Side::WHITE));
    EXPECT_FALSE(attacks::InCheck(board, Side::BLACK));
  }
  {
    Board board(Variant::STANDARD, "5k2/8/6q1/8/8/4K3/8/8 w - -");
    EXPECT_FALSE(attacks::InCheck(board, Side::WHITE));
    EXPECT_FALSE(attacks::InCheck(board, Side::BLACK));
  }
  {
    Board board(Variant::STANDARD, "5k2/8/6q1/8/5R2/4K3/8/8 b - -");
    EXPECT_TRUE(attacks::InCheck(board, Side::BLACK));
    EXPECT_FALSE(attacks::InCheck(board, Side::WHITE));
  }
  {
    Board board(Variant::STANDARD, "5k2/8/6q1/5n2/5R2/4K3/8/8 w - -");
    EXPECT_TRUE(attacks::InCheck(board, Side::WHITE));
    EXPECT_FALSE(attacks::InCheck(board, Side::BLACK));
  }
  {
    Board board(Variant::STANDARD, "5k2/8/8/8/8/8/1p6/2K5 w - -");
    EXPECT_TRUE(attacks::InCheck(board, Side::WHITE));
    EXPECT_FALSE(attacks::InCheck(board, Side::BLACK));
  }
  {
    Board board(Variant::STANDARD, "5k2/8/8/8/8/B7/8/2K5 w - -");
    EXPECT_TRUE(attacks::InCheck(board, Side::BLACK));
    EXPECT_FALSE(attacks::InCheck(board, Side::WHITE));
  }
}
