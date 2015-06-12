#include "board.h"
#include "common.h"
#include "egtb_util.h"
#include "fen.h"
#include "piece.h"

#include <iostream>
#include <string>
#include <vector>

#define EMPTY_BOARD "8/8/8/8/8/8/8/8 w - -"

namespace {
char side_as_char(Side side) {
  switch (side) {
    case Side::BLACK: return 'b';
    case Side::WHITE: return 'w';
    case Side::NONE: return '-';
  }
}

Side char_as_side(char c) {
  switch (c) {
    case 'b': return Side::BLACK;
    case 'w': return Side::WHITE;
    default: return Side::NONE;
  }
}
}

// static
char* EGTBUtil::ConvertFENToByteArray(const std::string& fen) {
  char num_pieces = FEN::NumPieces(fen);
  std::vector<FEN::PieceIndexInfo> pieces;
  FEN::GetPieces(fen, &pieces);
  unsigned size = pieces.size() * 2;
  char p[size];
  unsigned i = 0;
  for (const FEN::PieceIndexInfo& piece_index_info : pieces) {
    p[i] = PieceToChar(piece_index_info.piece);
    p[i + 1] = INDX(piece_index_info.row, piece_index_info.col);
    i += 2;
  }
  char player_side = side_as_char(FEN::PlayerColor(fen));
  char* board = new char[2 + size];
  board[0] = num_pieces;
  for (int i = 0; i < size; ++i) {
    board[1 + i] = p[i];
  }
  board[2 + size - 1] = player_side;
  return board;
}

// static
std::string EGTBUtil::ConvertByteArrayToFEN(char* b_array) {
  Board board(Variant::SUICIDE, EMPTY_BOARD);
  int size = b_array[0];
  unsigned j = 1;
  for (j = 1; j < (size * 2) + 1; j += 2) {
    Piece p = CharToPiece(b_array[j]);
    int index = b_array[j + 1];
    board.SetPiece(index, p);
  }
  Side side = char_as_side(b_array[j]);
  board.SetPlayerColor(side);
  return board.ParseIntoFEN();
}
