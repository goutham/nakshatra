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

namespace standard_chess {
namespace piece_value {
const int KING = 20000;
const int QUEEN = 900;
const int ROOK = 500;
const int BISHOP = 330;
const int KNIGHT = 320;
const int PAWN = 100;
constexpr int value[] = {0, KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
} // namespace piece_value

// clang-format off
constexpr int PST_KING[64] = {
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -20, -30, -30, -40, -40, -30, -30, -20,
  -10, -20, -20, -20, -20, -20, -20, -10,
   20,  20,   0,   0,   0,   0,  20,  20,
   20,  30,  10,   0,   0,  10,  30,  20
};

constexpr int PST_QUEEN[64] =  {
  -20, -10, -10, -5, -5, -10, -10, -20,
  -10,   0,   0,  0,  0,   0,   0, -10,
  -10,   0,   5,  5,  5,   5,   0, -10,
   -5,   0,   5,  5,  5,   5,   0,  -5,
    0,   0,   5,  5,  5,   5,   0,  -5,
  -10,   5,   5,  5,  5,   5,   0, -10,
  -10,   0,   5,  0,  0,   0,   0, -10,
  -20, -10, -10, -5, -5, -10, -10, -20
};

constexpr int PST_ROOK[64] =  {
   0,  0,  0,  0,  0,  0,  0,  0,
   5, 10, 10, 10, 10, 10, 10,  5,
  -5,  0,  0,  0,  0,  0,  0, -5,
  -5,  0,  0,  0,  0,  0,  0, -5,
  -5,  0,  0,  0,  0,  0,  0, -5,
  -5,  0,  0,  0,  0,  0,  0, -5,
  -5,  0,  0,  0,  0,  0,  0, -5,
   0,  0,  0,  5,  5,  0,  0,  0
};

constexpr int PST_BISHOP[64] =  {
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   5,   5,  10,  10,   5,   5, -10,
  -10,   0,  10,  10,  10,  10,   0, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   5,   0,   0,   0,   0,   5, -10,
  -20, -10, -10, -10, -10, -10, -10, -20
};

constexpr int PST_KNIGHT[64] =  {
  -50, -40, -30, -30, -30, -30, -40, -50,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  10,  15,  15,  10,   5, -30,
  -40, -20,   0,   5,   5,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50
};

constexpr int PST_PAWN[64] =  {
   0,  0,   0,   0,   0,   0,  0,  0,
  50, 50,  50,  50,  50,  50, 50, 50,
  10, 10,  20,  30,  30,  20, 10, 10,
   5,  5,  10,  25,  25,  10,  5,  5,
   0,  0,   0,  20,  20,   0,  0,  0,
   5, -5, -10,   0,   0, -10, -5,  5,
   5, 10,  10, -20, -20,  10, 10,  5,
   0,  0,   0,   0,   0,   0,  0,  0
};

constexpr int PST_KING_ENDGAME[64] = {
  -50, -40, -30, -20, -20, -30, -40, -50,
  -30, -20, -10,   0,   0, -10, -20, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -30,   0,   0,   0,   0, -30, -30,
  -50, -30, -30, -30, -30, -30, -30, -50
};
// clang-format on

constexpr const int* PST[] = {nullptr,    PST_KING,   PST_QUEEN, PST_ROOK,
                              PST_BISHOP, PST_KNIGHT, PST_PAWN};

template <Piece piece>
constexpr int PSTScore(U64 bb) {
  constexpr const int* pst = PST[PieceType(piece)];
  constexpr Side side = PieceSide(piece);
  int val = 0;
  while (bb) {
    const int sq = Lsb1(bb);
    int index = sq;
    if constexpr (side == Side::WHITE) {
      index = INDX(7 - ROW(sq), COL(sq));
    }
    val += pst[index];
    bb ^= (1ULL << sq);
  }
  return val;
}

constexpr int PSTVal(Side side, Piece piece, int sq) {
  return PST[PieceType(piece)]
            [(side == Side::WHITE) ? INDX(7 - ROW(sq), COL(sq)) : sq];
}

} // namespace standard_chess

namespace antichess {
namespace piece_value {
constexpr int KING = 10;
constexpr int QUEEN = 6;
constexpr int ROOK = 7;
constexpr int BISHOP = 3;
constexpr int KNIGHT = 3;
constexpr int PAWN = 2;
constexpr int value[] = {0, KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
} // namespace piece_value
} // namespace antichess

#endif
