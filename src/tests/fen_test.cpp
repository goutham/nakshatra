
#include "board.h"
#include "common.h"
#include "fen.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

struct PieceIndexInfo {
  Piece piece;
  int row;
  int col;
};

void GetPieces(const std::string& fen, std::vector<PieceIndexInfo>* pieces) {
  int row = 7;
  int col = 0;
  for (int i = 0; fen[i] != ' '; ++i) {
    if (fen[i] == '/') {
      --row;
      col = 0;
      continue;
    } else if (isalpha(fen[i])) {
      PieceIndexInfo p;
      p.piece = CharToPiece(fen[i]);
      p.row = row;
      p.col = col;
      pieces->push_back(p);
      ++col;
    } else if (isdigit(fen[i])) {
      col += CharToDigit(fen[i]);
    }
  }
}

TEST(FENTest, VerifyFEN) {
  std::string s = "2Q4r/2R5/8/8/1kP2N2/8/8/8 w - -";
  std::vector<PieceIndexInfo> pieces;
  GetPieces(s, &pieces);
  EXPECT_EQ(6, pieces.size());
  unsigned count = 0;
  for (const PieceIndexInfo& piece_index_info : pieces) {
    if (piece_index_info.piece == ROOK) {
      EXPECT_EQ(6, piece_index_info.row);
      EXPECT_EQ(2, piece_index_info.col);
      ++count;
    } else if (piece_index_info.piece == -KING) {
      EXPECT_EQ(3, piece_index_info.row);
      EXPECT_EQ(1, piece_index_info.col);
      ++count;
    } else if (piece_index_info.piece == QUEEN) {
      EXPECT_EQ(7, piece_index_info.row);
      EXPECT_EQ(2, piece_index_info.col);
      ++count;
    } else if (piece_index_info.piece == -ROOK) {
      EXPECT_EQ(7, piece_index_info.row);
      EXPECT_EQ(7, piece_index_info.col);
      ++count;
    } else if (piece_index_info.piece == PAWN) {
      EXPECT_EQ(3, piece_index_info.row);
      EXPECT_EQ(2, piece_index_info.col);
      ++count;
    } else if (piece_index_info.piece == KNIGHT) {
      EXPECT_EQ(3, piece_index_info.row);
      EXPECT_EQ(5, piece_index_info.col);
      ++count;
    }
  }
  EXPECT_EQ(6, count);
}
