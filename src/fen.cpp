#include "board.h"
#include "common.h"
#include "fen.h"
#include "move.h"
#include "piece.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using std::string;

namespace FEN {

void GetPieces(const string& fen, std::vector<PieceIndexInfo>* pieces) {
  int row = 7;
  int col = 0;
  for (int i = 0; fen[i] != ' '; ++i) {
    if (fen[i] == '/') {
      --row;
      col = 0;
      continue;
    } else if(isalpha(fen[i])) {
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

int NumPieces(const string& fen) {
  int count = 0;
  for (int i = 0; fen[i] != ' '; ++i) {
    if (isalpha(fen[i])) {
      ++count;
    }
  }
  return count;
}

Side PlayerColor(const string& fen) {
  unsigned i = 0;
  for (; fen[i] != ' '; ++i) { }
  ++i;
  if (fen[i] == 'w') {
    return Side::WHITE;
  } else if (fen[i] == 'b') {
    return Side::BLACK;
  } else {
    std::cout << "FATAL ERROR" << std::endl;
    exit(0);
  }
}

void MakeBoardArray(const string& fen, Piece board_array[]) {
  for (int i = 0; i < BOARD_SIZE; ++i) {
    board_array[i] = NULLPIECE;
  }
  int row = 7;
  int col = 0;
  unsigned index = 0;
  for (; index < fen.length(); ++index) {
    char c = fen[index];
    Piece piece = CharToPiece(c);
    if (IsValidPiece(piece)) {
      board_array[INDX(row, col)] = piece;
      ++col;
    }
    if (c == '/') {
      --row;
      col = 0;
    }
    if (isdigit(c)) {
      col += CharToDigit(c);
    }

    // If all rows are scanned
    if (c == ' ') {
      break;
    }
  }
}

Side PlayerToMove(const string& fen) {
  // Iterate until the first space is found.
  int index = 0;
  while (fen[index] != ' ') {
    ++index;
  }
  ++index;
  switch (fen[index]) {
    case 'w':
      return Side::WHITE;
    case 'b':
      return Side::BLACK;
    default:
      throw std::runtime_error("Malformed FEN string");
  }
}

unsigned char CastlingAvailability(const string& fen) {
  // Iterate until 2 spaces are found.
  int index = 0;
  int spaces_found = 0;
  while (spaces_found < 2) {
    if (fen[index] == ' ') {
      ++spaces_found;
    }
    ++index;
  }
  if (fen[index] == '-') {
    return 0;
  }
  unsigned char castle = 0;
  while (fen[index] != ' ') {
    switch (fen[index]) {
      case 'K': castle |= 0x1; break;
      case 'Q': castle |= 0x2; break;
      case 'k': castle |= 0x4; break;
      case 'q': castle |= 0x8; break;
    }
    ++index;
  }
  return castle;
}

int EnpassantIndex(const string& fen) {
  // Iterate until 3 spaces are found.
  int index = 0;
  int spaces_found = 0;
  while (spaces_found < 3) {
    if (fen[index] == ' ') {
      ++spaces_found;
    }
    ++index;
  }
  if (fen[index] == '-') {
    return -1;
  }
  return Move::index(fen.substr(index, 2));
}

string MakeFEN(const Piece board_array[],
               Side player_side,
               const unsigned char castle,
               int ep_index) {
  string fen;
  for (int i = 7; i >= 0; --i) {
    int empty_squares = 0;
    for (int j = 0; j < 8; ++j) {
      Piece piece = board_array[INDX(i, j)];
      if (piece == NULLPIECE) {
        ++empty_squares;
        continue;
      }
      if (empty_squares != 0) {
        fen += DigitToChar(empty_squares);
        empty_squares = 0;
      }
      fen += PieceToChar(piece);
    }
    if (empty_squares != 0) {
      fen += DigitToChar(empty_squares);
    }
    fen += '/';
  }
  fen.erase(fen.length() - 1);

  fen += ' ';

  if (player_side == Side::WHITE) {
    fen += 'w';
  } else {
    fen += 'b';
  }

  fen += ' ';

  string castling = "";
  if (castle & 0x1) {
    castling += 'K';
  }
  if (castle & 0x2) {
    castling += 'Q';
  }
  if (castle & 0x4) {
    castling += 'k';
  }
  if (castle & 0x8) {
    castling += 'q';
  }
  if (castling.empty()) {
    fen += '-';
  } else {
    fen += castling;
  }

  fen += ' ';
  fen += (ep_index >= 0) ? Move::file_rank(ep_index) : "-";

  return fen;
}

}
