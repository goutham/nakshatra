#include "board.h"
#include "movegen.h"
#include "movegen_suicide.h"
#include "movegen_normal.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>

using std::string;

class MoveGeneratorTest : public testing::Test {
 public:
  MoveGeneratorTest() {
  }
};

void DebugPrintMoveList(const MoveArray& move_array) {
  for (unsigned i = 0; i < move_array.size(); ++i) {
    std::cerr << move_array.get(i).str() << std::endl;
  }
}

TEST_F(MoveGeneratorTest, VerifyValidMove) {
  Board board(SUICIDE);
  MoveGeneratorSuicide movegen(board);

  Move move("e2e3");
  EXPECT_EQ(true, movegen.IsValidMove(move));

  Move move2("e2e5");
  EXPECT_EQ(false, movegen.IsValidMove(move2));
}

TEST_F(MoveGeneratorTest, VerifyWhitePawnPromotion) {
  string initial = "8/1P6/8/8/8/8/8/8 w - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(5, move_array.size());

  string exp[] = {
    "1Q6/8/8/8/8/8/8/8 b - -",
    "1R6/8/8/8/8/8/8/8 b - -",
    "1B6/8/8/8/8/8/8/8 b - -",
    "1K6/8/8/8/8/8/8/8 b - -",
    "1N6/8/8/8/8/8/8/8 b - -",
  };

  for (unsigned i = 0; i < move_array.size(); ++i) {
    board.MakeMove(move_array.get(i));
    bool flag = false;
    for (int i = 0; i < 5; ++i) {
      if (exp[i] == board.ParseIntoFEN()) {
        flag = true;
        break;
      }
    }
    EXPECT_TRUE(flag);
    board.UnmakeLastMove();
    EXPECT_EQ(initial, board.ParseIntoFEN());
  }
}

TEST_F(MoveGeneratorTest, VerifyBlackPawnPromotion) {
  string initial = "8/8/8/8/8/8/1p6/8 b - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(5, move_array.size());

  string exp[] = {
    "8/8/8/8/8/8/8/1q6 w - -",
    "8/8/8/8/8/8/8/1r6 w - -",
    "8/8/8/8/8/8/8/1k6 w - -",
    "8/8/8/8/8/8/8/1b6 w - -",
    "8/8/8/8/8/8/8/1n6 w - -",
  };

  for (unsigned i = 0; i < move_array.size(); ++i) {
    board.MakeMove(move_array.get(i));
    bool flag = false;
    for (int j = 0; j < 5; ++j) {
      if (exp[j] == board.ParseIntoFEN()) {
        flag = true;
        break;
      }
    }
    EXPECT_TRUE(flag);
    board.UnmakeLastMove();
    EXPECT_EQ(initial, board.ParseIntoFEN());
  }
}

TEST_F(MoveGeneratorTest, VerifyWhitePawnPromotionHit) {
  string initial = "r1q5/1P6/8/8/8/8/8/8 w - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(10, move_array.size());

  string exp[] = {
    "r1Q5/8/8/8/8/8/8/8 b - -",
    "r1R5/8/8/8/8/8/8/8 b - -",
    "r1B5/8/8/8/8/8/8/8 b - -",
    "r1K5/8/8/8/8/8/8/8 b - -",
    "r1N5/8/8/8/8/8/8/8 b - -",
    "Q1q5/8/8/8/8/8/8/8 b - -",
    "R1q5/8/8/8/8/8/8/8 b - -",
    "B1q5/8/8/8/8/8/8/8 b - -",
    "K1q5/8/8/8/8/8/8/8 b - -",
    "N1q5/8/8/8/8/8/8/8 b - -",
  };

  for (unsigned i = 0; i < move_array.size(); ++i) {
    board.MakeMove(move_array.get(i));
    bool flag = false;
    for (int j = 0; j < 10; ++j) {
      if (exp[j] == board.ParseIntoFEN()) {
        flag = true;
        break;
      }
    }
    EXPECT_TRUE(flag);
    board.UnmakeLastMove();
    EXPECT_EQ(initial, board.ParseIntoFEN());
  }
}

TEST_F(MoveGeneratorTest, VerifyBlackPawnPromotionHit) {
  string initial = "8/8/8/8/8/8/1p6/R1Q5 b - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(10, move_array.size());

  string exp[] = {
    "8/8/8/8/8/8/8/q1Q5 w - -",
    "8/8/8/8/8/8/8/r1Q5 w - -",
    "8/8/8/8/8/8/8/b1Q5 w - -",
    "8/8/8/8/8/8/8/k1Q5 w - -",
    "8/8/8/8/8/8/8/n1Q5 w - -",
    "8/8/8/8/8/8/8/R1q5 w - -",
    "8/8/8/8/8/8/8/R1b5 w - -",
    "8/8/8/8/8/8/8/R1r5 w - -",
    "8/8/8/8/8/8/8/R1k5 w - -",
    "8/8/8/8/8/8/8/R1n5 w - -",
  };

  for (unsigned i = 0; i < move_array.size(); ++i) {
    board.MakeMove(move_array.get(i));
    bool flag = false;
    for (int j = 0; j < 10; ++j) {
      if (exp[j] == board.ParseIntoFEN()) {
        flag = true;
        break;
      }
    }
    EXPECT_TRUE(flag);
    board.UnmakeLastMove();
    EXPECT_EQ(initial, board.ParseIntoFEN());
  }
}

TEST_F(MoveGeneratorTest, VerifyPawnFirstMove) {
  string initial = "8/1p6/1p6/8/8/8/8/8 b - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(1, move_array.size());
  board.MakeMove(move_array.get(0));

  EXPECT_EQ("8/1p6/8/1p6/8/8/8/8 w - -", board.ParseIntoFEN());
}

TEST_F(MoveGeneratorTest, VerifyEnpassantMoves) {
  string initial = "8/1p6/8/2P5/8/8/8/8 b - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(2, move_array.size());

  string exp1[] = {
    "8/8/1p6/2P5/8/8/8/8 w - -",
    "8/8/8/1pP5/8/8/8/8 w - b6"
  };
  int index = -1;
  for (int i = 0; i < 2; ++i) {
    board.MakeMove(move_array.get(i));
    if (board.ParseIntoFEN() == exp1[1]) {
      index = i;
    }
    board.UnmakeLastMove();
  }
  ASSERT_TRUE(index > -1);
  board.MakeMove(move_array.get(index));

  MoveGeneratorSuicide movegen2(board);
  MoveArray move_array2;
  movegen2.GenerateMoves(&move_array2);
  EXPECT_EQ(1, move_array2.size());

  board.MakeMove(move_array2.get(0));
  string exp2 = "8/8/1P6/8/8/8/8/8 b - -";
  EXPECT_EQ(exp2, board.ParseIntoFEN());

  board.UnmakeLastMove();
  EXPECT_EQ(exp1[1], board.ParseIntoFEN());
}

TEST_F(MoveGeneratorTest, VerifyEnpassantMoves2) {
  string initial = "8/8/8/8/2p5/8/1P6/8 w - -";
  Board board(SUICIDE, initial);

  MoveGeneratorSuicide movegen(board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  EXPECT_EQ(2, move_array.size());

  string exp1[] = {
    "8/8/8/8/8/2p5/1P6/8 b - -",
    "8/8/8/8/1Pp5/8/8/8 b - b3"
  };
  int index = -1;
  for (int i = 0; i < 2; ++i) {
    board.MakeMove(move_array.get(i));
    if (board.ParseIntoFEN() == exp1[1]) {
      index = i;
    }
    board.UnmakeLastMove();
  }
  ASSERT_TRUE(index > -1);
  board.MakeMove(move_array.get(index));

  MoveGeneratorSuicide movegen2(board);
  MoveArray move_array2;
  movegen2.GenerateMoves(&move_array2);
  EXPECT_EQ(1, move_array2.size());

  board.MakeMove(move_array2.get(0));
  string exp2 = "8/8/8/8/8/1p6/8/8 w - -";
  EXPECT_EQ(exp2, board.ParseIntoFEN());

  board.UnmakeLastMove();
  EXPECT_EQ(exp1[1], board.ParseIntoFEN());
}

TEST_F(MoveGeneratorTest, VerifyInitialMoves) {
  Board board(NORMAL);
  MoveGeneratorNormal movegen(&board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  static const string valid_moves[] = {
      "a2a3",
      "b2b3",
      "c2c3",
      "d2d3",
      "e2e3",
      "f2f3",
      "g2g3",
      "h2h3",
      "a2a4",
      "b2b4",
      "c2c4",
      "d2d4",
      "e2e4",
      "f2f4",
      "g2g4",
      "h2h4",
      "b1a3",
      "b1c3",
      "g1f3",
      "g1h3"
  };
  static const unsigned num_valid_moves = 20;
  EXPECT_EQ(num_valid_moves, move_array.size());
  for (int i = 0; i < num_valid_moves; ++i) {
    const Move move(valid_moves[i]);
    bool found = false;
    for (int i = 0; i < move_array.size(); ++i) {
      if (move_array.get(i) == move) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  }
}

TEST_F(MoveGeneratorTest, VerifyPinnedPieceMoves) {
  Board board(NORMAL, "8/8/4r3/b7/3b4/2Q2p2/4P3/4K3 w - -");
  MoveGeneratorNormal movegen(&board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  static const string valid_moves[] = {
      "e2e3",
      "e2e4",
      "c3d2",
      "c3b4",
      "c3a5",
      "e1d1",
      "e1f1",
      "e1d2",
  };
  static const unsigned num_valid_moves = 8;
  EXPECT_EQ(num_valid_moves, move_array.size());
  for (int i = 0; i < num_valid_moves; ++i) {
    const Move move(valid_moves[i]);
    bool found = false;
    for (int j = 0; j < move_array.size(); ++j) {
      if (move_array.get(j) == move) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  }
}

TEST_F(MoveGeneratorTest, VerifyMovesUnderCheck) {
  Board board(NORMAL, "rnb1kbnr/pppp1p1p/6p1/4P3/1q2P3/8/PPPK1PPP/RNBQ1BNR w KQkq -");
  board.DebugPrintBoard();
  MoveGeneratorNormal movegen(&board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  DebugPrintMoveList(move_array);
  static const string valid_moves[] = {
      "d2e2",
      "d2d3",
      "d2e3",
      "c2c3",
      "b1c3"
  };
  static const unsigned num_valid_moves = 5;
  EXPECT_EQ(num_valid_moves, move_array.size());
  for (int i = 0; i < num_valid_moves; ++i) {
    const Move move(valid_moves[i]);
    bool found = false;
    for (int j = 0; j < move_array.size(); ++j) {
      if (move_array.get(j) == move) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  }
}

TEST_F(MoveGeneratorTest, VerifyMovesUnderCheck2) {
  Board board(NORMAL, "rnb1kbnr/pppp1ppp/4p3/8/3P4/2P5/PP1KPqPP/RNBQ1BNR w KQkq -");
  MoveGeneratorNormal movegen(&board);
  MoveArray move_array;
  movegen.GenerateMoves(&move_array);
  static const string valid_moves[] = {
      "a2a3",
      "b2b3",
      "g2g3",
      "h2h3",
      "c3c4",
      "d4d5",
      "a2a4",
      "b2b4",
      "g2g4",
      "h2h4",
      "d1e1",
      "d1c2",
      "d1b3",
      "d1a4",
      "b1a3",
      "g1f3",
      "g1h3",
      "d2c2",
      "d2d3"
  };
  static const unsigned num_valid_moves = 19;
  EXPECT_EQ(num_valid_moves, move_array.size());
  for (int i = 0; i < num_valid_moves; ++i) {
    const Move move(valid_moves[i]);
    bool found = false;
    for (int j = 0; j < move_array.size(); ++j) {
      if (move_array.get(j) == move) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  }
}

TEST_F(MoveGeneratorTest, VerifySlidingAttackMaps) {
  Board board(SUICIDE,"R7/1P1PPP1P/N2B3R/2P1P3/PP4P1/3p1pp1/1p1ppp2/1n1qkb2 w - -");
  U64 attack_map = Attacks(QUEEN, INDX(7, 6), board);
  EXPECT_EQ(13826121500723249152ULL, attack_map);

  attack_map = Attacks(QUEEN, INDX(3, 4), board);
  EXPECT_EQ(36666756130410496ULL, attack_map);

  attack_map = Attacks(QUEEN, INDX(0, 0), board);
  EXPECT_EQ(16843522ULL, attack_map);

  attack_map = Attacks(QUEEN, INDX(7, 7), board);
  EXPECT_EQ(9205392891436859392ULL, attack_map);

  attack_map = Attacks(QUEEN, INDX(6, 2), board);
  EXPECT_EQ(1011636480935723008ULL, attack_map);

  attack_map = Attacks(QUEEN, INDX(2, 2), board);
  EXPECT_EQ(86134951428ULL, attack_map);

  attack_map = Attacks(ROOK, INDX(3, 5), board);
  EXPECT_EQ(9042522644938752ULL, attack_map);

  attack_map = Attacks(BISHOP, INDX(3, 5), board);
  EXPECT_EQ(141081090983936ULL, attack_map);
}
