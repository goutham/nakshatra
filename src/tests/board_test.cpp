// To test private stuff in class Board. Should be OK in a unit test.
#define private public

#include "board.h"
#include "common.h"
#include "move.h"
#include "movegen.h"
#include "movegen_suicide.h"
#include "piece.h"
#include "san.h"
#include "zobrist.h"

#include <gtest/gtest.h>

#define _ NULLPIECE

using std::string;

class BoardTest : public testing::Test {
 public:
  BoardTest() {
    movegen::InitializeIfNeeded();
    zobrist::InitializeIfNeeded();
  }
};

TEST_F(BoardTest, VerifyFENInitializedBoard) {
  Piece expected_board[8][8] = {
    { ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK },
    { PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN },
    { _, _, _, _, _, _, _, _ },
    { _, _, _, _, _, _, _, _ },
    { _, _, _, _, _, _, _, _ },
    { _, _, _, _, _, _, _, _ },
    { -PAWN, -PAWN, -PAWN, -PAWN, -PAWN, -PAWN, -PAWN, -PAWN },
    { -ROOK, -KNIGHT, -BISHOP, -QUEEN, -KING, -BISHOP, -KNIGHT, -ROOK },
  };

  Board board(SUICIDE);
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      EXPECT_EQ(expected_board[i][j], board.PieceAt(i, j));
    }
  }
  EXPECT_EQ(Side::WHITE, board.SideToMove());
}

TEST_F(BoardTest, VerifyFENFunctionality) {
  string expected_fen = "r2qr1k1/ppbn1pp1/4bn1p/PN1pp3/1P2P3/3P1N2/2Q1BPPP/R1B2RK1 b - -";
  Board board(SUICIDE, expected_fen);
  string actual_fen = board.ParseIntoFEN();
  EXPECT_EQ(expected_fen, actual_fen);
  EXPECT_EQ(Side::BLACK, board.SideToMove());
}

TEST_F(BoardTest, VerifyPieceMovements) {
  string init_board = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - -";
  Board board(SUICIDE, init_board);

  board.MakeMove(Move("e2e4"));
  string after_e4 = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3";
  EXPECT_EQ(after_e4, board.ParseIntoFEN());
  EXPECT_EQ(Side::BLACK, board.SideToMove());

  board.MakeMove(Move("e7e5"));
  string after_e5 = "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w - e6";
  EXPECT_EQ(after_e5, board.ParseIntoFEN());
  EXPECT_EQ(Side::WHITE, board.SideToMove());

  board.UnmakeLastMove();
  EXPECT_EQ(after_e4, board.ParseIntoFEN());
  EXPECT_EQ(Side::BLACK, board.SideToMove());

  board.UnmakeLastMove();
  EXPECT_EQ(init_board, board.ParseIntoFEN());
  EXPECT_EQ(Side::WHITE, board.SideToMove());

  EXPECT_FALSE(board.UnmakeLastMove());
}

TEST_F(BoardTest, VerifyNumPieces) {
  Board board(SUICIDE, "8/8/1r1q4/2P5/8/8/8/8 w - -");
  board.DebugPrintBoard();
  EXPECT_EQ(1, board.NumPieces(Side::WHITE));
  EXPECT_EQ(2, board.NumPieces(Side::BLACK));

  board.MakeMove(Move("c5d6"));
  board.DebugPrintBoard();
  EXPECT_EQ(1, board.NumPieces(Side::WHITE));
  EXPECT_EQ(1, board.NumPieces(Side::BLACK));

  board.UnmakeLastMove();
  EXPECT_EQ(1, board.NumPieces(Side::WHITE));
  EXPECT_EQ(2, board.NumPieces(Side::BLACK));
}

TEST_F(BoardTest, Pawn2SpaceMoves) {
  Board board(SUICIDE, "8/3p/8/8/8/8/1P6/8 w - -");
  EXPECT_TRUE(board.EnpassantTarget() == -1);
  board.MakeMove(Move("b2b4"));
  EXPECT_TRUE(board.EnpassantTarget() != -1);
  board.MakeMove(Move("d7d6"));
  EXPECT_TRUE(board.EnpassantTarget() == -1);
  board.UnmakeLastMove();
  EXPECT_TRUE(board.EnpassantTarget() != -1);
  board.UnmakeLastMove();
  EXPECT_TRUE(board.EnpassantTarget() == -1);
}

TEST_F(BoardTest, EnpassantCapture) {
  Board board(SUICIDE, "8/8/8/8/2p5/8/1P6/8 w - -");
  EXPECT_TRUE(board.EnpassantTarget() == -1);
  board.MakeMove(Move("b2b4"));
  EXPECT_TRUE(board.EnpassantTarget() != -1);
  board.MakeMove(Move("c4b3"));
  EXPECT_EQ("8/8/8/8/8/1p6/8/8 w - -", board.ParseIntoFEN());
  EXPECT_TRUE(board.EnpassantTarget() == -1);
  board.UnmakeLastMove();
  EXPECT_EQ("8/8/8/8/1Pp5/8/8/8 b - b3", board.ParseIntoFEN());
  EXPECT_TRUE(board.EnpassantTarget() != -1);
  board.UnmakeLastMove();
  EXPECT_EQ("8/8/8/8/2p5/8/1P6/8 w - -", board.ParseIntoFEN());
  EXPECT_TRUE(board.EnpassantTarget() == -1);
}

TEST_F(BoardTest, ZobristTest1) {
  string init_board = "8/8/8/8/8/8/1P6/8 w - -";
  Board board(SUICIDE, init_board);
  board.MakeMove(Move("b2b4"));
  U64 z1 = board.ZobristKey();
  string b1 = board.ParseIntoFEN();

  board.UnmakeLastMove();
  board.MakeMove(Move("b2b3"));

  board.UnmakeLastMove();
  board.MakeMove(Move("b2b4"));
  U64 z3 = board.ZobristKey();
  string b3 = board.ParseIntoFEN();

  EXPECT_TRUE(z1 == z3);
  EXPECT_TRUE(b1 == b3);
}

// Verifies that a 2 space pawn move creates a different zobrist hash value
// compared to same position of board without 2 space pawn move. Further also
// verifies that this does not have any side-effect on subsequent moves.
TEST_F(BoardTest, Zobrist2) {
  Board board(SUICIDE, "8/1p6/8/8/8/8/1P6/8 w - -");
  board.MakeMove(Move("b2b4"));
  U64 z1 = board.ZobristKey();
  string b1 = board.ParseIntoFEN();
  board.MakeMove(Move("b7b6"));
  U64 z2 = board.ZobristKey();
  string b2 = board.ParseIntoFEN();
  board = Board(SUICIDE, "8/1p6/8/8/8/1P6/8/8 w - -");
  board.MakeMove(Move("b3b4"));
  U64 z3 = board.ZobristKey();
  string b3 = board.ParseIntoFEN();
  board.MakeMove(Move("b7b6"));
  U64 z4 = board.ZobristKey();
  string b4 = board.ParseIntoFEN();

  EXPECT_FALSE(z1 == z3);
  EXPECT_TRUE(b1 != b3);
  EXPECT_TRUE(z2 == z4);
  EXPECT_TRUE(b2 == b4);
}

TEST_F(BoardTest, CastlingTest) {
  string init_board = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
  Board board(NORMAL, init_board);
  board.MakeMove(Move("e2e4"));
  board.MakeMove(Move("e7e5"));
  board.MakeMove(Move("f1c4"));
  board.MakeMove(Move("f8c5"));
  board.MakeMove(Move("g1f3"));
  board.MakeMove(Move("g8f6"));

  U64 z_before_white_castling = board.ZobristKey();
  string fen_before_white_castling = board.ParseIntoFEN();
  EXPECT_EQ("rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -",
            fen_before_white_castling);
  EXPECT_TRUE(board.CanCastle(KING));
  EXPECT_TRUE(board.CanCastle(QUEEN));

  // White - king side castling.
  board.MakeMove(Move("e1g1"));
  U64 z_after_white_castling = board.ZobristKey();
  string fen_after_white_castling = board.ParseIntoFEN();
  EXPECT_EQ("rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 b kq -",
            fen_after_white_castling);
  // Black side castling must be available.
  EXPECT_TRUE(board.CanCastle(KING));
  EXPECT_TRUE(board.CanCastle(QUEEN));

  board.UnmakeLastMove();
  EXPECT_EQ(fen_before_white_castling, board.ParseIntoFEN());
  EXPECT_EQ(z_before_white_castling, board.ZobristKey());

  board.MakeMove(Move("e1g1"));

  // Black - king side castling.
  board.MakeMove(Move("e8g8"));
  U64 z_after_black_castling = board.ZobristKey();
  string fen_after_black_castling = board.ParseIntoFEN();
  EXPECT_EQ("rnbq1rk1/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 w - -",
            fen_after_black_castling);
  // White side castling unavailable.
  EXPECT_FALSE(board.CanCastle(KING));
  EXPECT_FALSE(board.CanCastle(QUEEN));

  board.UnmakeLastMove();
  EXPECT_EQ(fen_after_white_castling, board.ParseIntoFEN());
  EXPECT_EQ(z_after_white_castling, board.ZobristKey());

  board.UnmakeLastMove();

  board.MakeMove(Move("d1e2"));
  board.MakeMove(Move("d8e7"));
  board.MakeMove(Move("b1c3"));
  board.MakeMove(Move("b8c6"));
  board.MakeMove(Move("d2d3"));
  board.MakeMove(Move("d7d6"));
  board.MakeMove(Move("c1d2"));
  board.MakeMove(Move("c8d7"));

  EXPECT_EQ("r3k2r/pppbqppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/R3K2R w KQkq -",
            board.ParseIntoFEN());

  U64 z1 = board.ZobristKey();
  board.MakeMove(Move("e1c1"));
  EXPECT_EQ("r3k2r/pppbqppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/2KR3R b kq -",
            board.ParseIntoFEN());
  U64 z2 = board.ZobristKey();
  board.MakeMove(Move("e8c8"));
  EXPECT_EQ("2kr3r/pppbqppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/2KR3R w - -",
            board.ParseIntoFEN());

  board.UnmakeLastMove();
  EXPECT_EQ("r3k2r/pppbqppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/2KR3R b kq -",
            board.ParseIntoFEN());
  EXPECT_EQ(z2, board.ZobristKey());
  board.UnmakeLastMove();
  EXPECT_EQ("r3k2r/pppbqppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/R3K2R w KQkq -",
            board.ParseIntoFEN());
  EXPECT_EQ(z1, board.ZobristKey());
}

TEST_F(BoardTest, BitBoardVerification) {
  Board board(SUICIDE);

  U64 black, white, white_pawns, white_kings, white_queens, white_rooks,
      white_bishops, white_knights, black_pawns, black_kings, black_queens,
      black_rooks, black_bishops, black_knights;

  auto get_bitboards = [&] {
    black = board.BitBoard(Side::BLACK);
    white = board.BitBoard(Side::WHITE);
    white_pawns = board.BitBoard(PAWN);
    white_kings = board.BitBoard(KING);
    white_queens = board.BitBoard(QUEEN);
    white_rooks = board.BitBoard(ROOK);
    white_bishops = board.BitBoard(BISHOP);
    white_knights = board.BitBoard(KNIGHT);
    black_pawns = board.BitBoard(-PAWN);
    black_kings = board.BitBoard(-KING);
    black_queens = board.BitBoard(-QUEEN);
    black_rooks = board.BitBoard(-ROOK);
    black_bishops = board.BitBoard(-BISHOP);
    black_knights = board.BitBoard(-KNIGHT);
  };

  get_bitboards();
  EXPECT_EQ(65280ULL, white_pawns);
  EXPECT_EQ(16ULL, white_kings);
  EXPECT_EQ(8ULL, white_queens);
  EXPECT_EQ(129ULL, white_rooks);
  EXPECT_EQ(36ULL, white_bishops);
  EXPECT_EQ(66ULL, white_knights);
  EXPECT_EQ(71776119061217280ULL, black_pawns);
  EXPECT_EQ(1152921504606846976ULL, black_kings);
  EXPECT_EQ(576460752303423488ULL, black_queens);
  EXPECT_EQ(2594073385365405696ULL, black_bishops);
  EXPECT_EQ(4755801206503243776ULL, black_knights);
  EXPECT_EQ(9295429630892703744ULL, black_rooks);

  board.MakeMove(Move("e2e4"));
  get_bitboards();
  EXPECT_EQ(268496640ULL, white_pawns);

  board.MakeMove(Move("d7d5"));
  get_bitboards();
  EXPECT_EQ(69524353607270400ULL, black_pawns);

  board.MakeMove(Move("e4d5"));
  get_bitboards();
  EXPECT_EQ(34359799552ULL, white_pawns);
  EXPECT_EQ(69524319247532032ULL, black_pawns);

  board.MakeMove(Move("e7e5"));
  get_bitboards();
  EXPECT_EQ(34359799552ULL, white_pawns);
  EXPECT_EQ(65020788339638272ULL, black_pawns);

  // Enpassant.
  board.MakeMove(Move("d5e6"));
  get_bitboards();
  EXPECT_EQ(17592186105600ULL, white_pawns);
  EXPECT_EQ(65020719620161536ULL, black_pawns);

  board.MakeMove(Move("d8d2"));
  get_bitboards();
  EXPECT_EQ(17592186103552ULL, white_pawns);
  EXPECT_EQ(65020719620161536ULL, black_pawns);
  EXPECT_EQ(2048ULL, black_queens);

  board.MakeMove(Move("e1d2"));
  get_bitboards();
  EXPECT_EQ(0ULL, black_queens);
  EXPECT_EQ(2048ULL, white_kings);
}

TEST_F(BoardTest, PawnPromotions) {
  Board board(SUICIDE, "8/8/8/P7/8/2p5/3ppp2/8 b - -");

  U64 white_pawns, black_pawns, black_kings, black_rooks, white_queens,
      bitboard, black, white;

  auto get_bitboards = [&] {
    white_pawns = board.BitBoard(PAWN);
    black_pawns = board.BitBoard(-PAWN);
    black_kings = board.BitBoard(-KING);
    black_rooks = board.BitBoard(-ROOK);
    white_queens = board.BitBoard(QUEEN);
    bitboard = board.BitBoard();
    black = board.BitBoard(Side::BLACK);
    white = board.BitBoard(Side::WHITE);
  };

  get_bitboards();
  EXPECT_EQ(4294967296ULL, white_pawns);
  EXPECT_EQ(276480ULL, black_pawns);
  EXPECT_EQ(4295243776ULL, bitboard);
  EXPECT_EQ(276480ULL, black);
  EXPECT_EQ(4294967296ULL, white);
  EXPECT_EQ(0ULL, black_kings);
  EXPECT_EQ(0ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("d2d1k"));
  get_bitboards();
  EXPECT_EQ(4294967296ULL, white_pawns);
  EXPECT_EQ(274432ULL, black_pawns);
  EXPECT_EQ(4295241736ULL, bitboard);
  EXPECT_EQ(274440ULL, black);
  EXPECT_EQ(4294967296ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(0ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("a5a6"));
  get_bitboards();
  EXPECT_EQ(1099511627776ULL, white_pawns);
  EXPECT_EQ(274432ULL, black_pawns);
  EXPECT_EQ(1099511902216ULL, bitboard);
  EXPECT_EQ(274440ULL, black);
  EXPECT_EQ(1099511627776ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(0ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("c3c2"));
  get_bitboards();
  EXPECT_EQ(1099511627776ULL, white_pawns);
  EXPECT_EQ(13312ULL, black_pawns);
  EXPECT_EQ(1099511641096ULL, bitboard);
  EXPECT_EQ(13320ULL, black);
  EXPECT_EQ(1099511627776ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(0ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("a6a7"));
  get_bitboards();
  EXPECT_EQ(281474976710656ULL, white_pawns);
  EXPECT_EQ(13312ULL, black_pawns);
  EXPECT_EQ(281474976723976ULL, bitboard);
  EXPECT_EQ(13320ULL, black);
  EXPECT_EQ(281474976710656ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(0ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("c2c1R"));
  get_bitboards();
  EXPECT_EQ(281474976710656ULL, white_pawns);
  EXPECT_EQ(12288ULL, black_pawns);
  EXPECT_EQ(281474976722956ULL, bitboard);
  EXPECT_EQ(12300ULL, black);
  EXPECT_EQ(281474976710656ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(4ULL, black_rooks);
  EXPECT_EQ(0ULL, white_queens);

  board.MakeMove(Move("a7a8q"));
  get_bitboards();
  EXPECT_EQ(0ULL, white_pawns);
  EXPECT_EQ(12288ULL, black_pawns);
  EXPECT_EQ(72057594037940236ULL, bitboard);
  EXPECT_EQ(12300ULL, black);
  EXPECT_EQ(72057594037927936ULL, white);
  EXPECT_EQ(8ULL, black_kings);
  EXPECT_EQ(4ULL, black_rooks);
  EXPECT_EQ(72057594037927936ULL, white_queens);
}

TEST_F(BoardTest, SANTest1) {
  Board board(SUICIDE);
  EXPECT_EQ("e3", SAN(board, Move("e2e3")));
  EXPECT_EQ("g4", SAN(board, Move("g2g4")));
  EXPECT_EQ("Nf3", SAN(board, Move("g1f3")));
  EXPECT_EQ("Nh3", SAN(board, Move("g1h3")));
  EXPECT_EQ("Na3", SAN(board, Move("b1a3")));
  EXPECT_EQ("Nc3", SAN(board, Move("b1c3")));
}

TEST_F(BoardTest, SANTest2) {
  Board board(NORMAL, "8/2k5/4r3/3P2N1/8/R7/7p/3R2N1 w - -");
  EXPECT_EQ("N1f3", SAN(board, Move("g1f3")));
  EXPECT_EQ("N5f3", SAN(board, Move("g5f3")));
  EXPECT_EQ("Ne2", SAN(board, Move("g1e2")));
  EXPECT_EQ("Rdd3", SAN(board, Move("d1d3")));
  EXPECT_EQ("dxe6", SAN(board, Move("d5e6")));
  EXPECT_EQ("Nxe6", SAN(board, Move("g5e6")));
  EXPECT_EQ("Raa1", SAN(board, Move("a3a1")));
  EXPECT_EQ("Rad3", SAN(board, Move("a3d3")));
}

TEST_F(BoardTest, SANTest3) {
  Board board(NORMAL, "8/2k5/4r3/3P2N1/8/R7/7p/3R2N1 b - -");
  EXPECT_EQ("h1Q", SAN(board, Move("h2h1q")));
  EXPECT_EQ("hxg1Q", SAN(board, Move("h2g1q")));
  EXPECT_EQ("Kb7", SAN(board, Move("c7b7")));
}

TEST_F(BoardTest, SANToMoveTest1) {
  Board board(SUICIDE);
  movegen::MoveGeneratorSuicide movegen(board);
  EXPECT_EQ("e2e3", SANToMove("e3", board, &movegen).str());
  EXPECT_EQ("g2g4", SANToMove("g4", board, &movegen).str());
  EXPECT_EQ("g1f3", SANToMove("Nf3", board, &movegen).str());
  EXPECT_EQ("g1h3", SANToMove("Nh3", board, &movegen).str());
  EXPECT_EQ("b1a3", SANToMove("Na3", board, &movegen).str());
  EXPECT_EQ("b1c3", SANToMove("Nc3", board, &movegen).str());
}
