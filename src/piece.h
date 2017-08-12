#ifndef PIECE_H
#define PIECE_H

#include "common.h"

#include <cctype>

typedef int Piece;

const Piece NULLPIECE = 0;
const Piece KING = 1;
const Piece QUEEN = 2;
const Piece ROOK = 3;
const Piece BISHOP = 4;
const Piece KNIGHT = 5;
const Piece PAWN = 6;

constexpr bool IsValidPiece(Piece piece) {
  return piece != NULLPIECE && -7 < piece && piece < 7;
}

constexpr Side PieceSide(Piece piece) {
  return piece == NULLPIECE ? Side::NONE
                            : (piece < 0 ? Side::BLACK : Side::WHITE);
}

constexpr Piece PieceType(Piece piece) { return piece < 0 ? -piece : piece; }

constexpr Piece PieceOfSide(Piece piece, Side side) {
  return side == Side::WHITE ? PieceType(piece) : -PieceType(piece);
}

// Maps a piece to an index between 0 and 11 (inclusive).
constexpr int PieceIndex(const Piece piece) {
  return piece < 0 ? (5 - piece) : (piece - 1);
}

// Returns NULLPIECE if 'c' does not correspond to valid piece characters.
inline Piece CharToPiece(char c) {
  Piece piece = NULLPIECE;
  switch (toupper(c)) {
  case 'K':
    piece = KING;
    break;
  case 'Q':
    piece = QUEEN;
    break;
  case 'B':
    piece = BISHOP;
    break;
  case 'R':
    piece = ROOK;
    break;
  case 'N':
    piece = KNIGHT;
    break;
  case 'P':
    piece = PAWN;
    break;
  }
  return (piece == NULLPIECE) ? piece : (isupper(c) ? piece : -piece);
}

// If piece is NULLPIECE, returns ' '.
inline char PieceToChar(Piece piece) {
  char c = ' ';
  switch (PieceType(piece)) {
  case KING:
    c = 'K';
    break;
  case QUEEN:
    c = 'Q';
    break;
  case BISHOP:
    c = 'B';
    break;
  case ROOK:
    c = 'R';
    break;
  case KNIGHT:
    c = 'N';
    break;
  case PAWN:
    c = 'P';
    break;
  }
  return isalpha(c) ? (piece < NULLPIECE ? tolower(c) : c) : c;
}

#endif
