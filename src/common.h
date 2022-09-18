#ifndef COMMON_H
#define COMMON_H

#include <cctype>
#include <climits>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#define BOARD_SIZE 64
#define MAX_DEPTH 100
#define INF SHRT_MAX
#define WIN (SHRT_MAX - 1)
#define DRAW 0
#define UNKNOWN -1

enum class Variant {
  STANDARD,
  ANTICHESS,
  SUICIDE // FICS rules
};

enum class Side { NONE, WHITE, BLACK };

enum class NodeType { FAIL_HIGH_NODE, FAIL_LOW_NODE, EXACT_NODE };

typedef uint64_t U64;
typedef int Piece;

constexpr Piece NULLPIECE = 0;
constexpr Piece KING = 1;
constexpr Piece QUEEN = 2;
constexpr Piece ROOK = 3;
constexpr Piece BISHOP = 4;
constexpr Piece KNIGHT = 5;
constexpr Piece PAWN = 6;

extern std::ostream nullstream;

// Column numbers of files.
constexpr int FILE_A = 0;
constexpr int FILE_B = 1;
constexpr int FILE_C = 2;
constexpr int FILE_D = 3;
constexpr int FILE_E = 4;
constexpr int FILE_F = 5;
constexpr int FILE_G = 6;
constexpr int FILE_H = 7;

inline int Lsb1(const U64 v) { return __builtin_ctzll(v); }

// Number of set bits in a U64 integer.
inline int PopCount(U64 v) { return __builtin_popcountll(v); }

constexpr int CharToDigit(const char c) {
  return (c >= '0' && c <= '9')
             ? (c - 48)
             : throw std::logic_error("Expected char between '0' and '9'");
}

constexpr unsigned INDX(const unsigned row, const unsigned col) {
  return row * 8 + col;
}

constexpr unsigned INDX(const char* sq) {
  return INDX(CharToDigit(sq[1]) - 1, sq[0] - 'a');
}

constexpr unsigned ROW(const unsigned index) { return index / 8; }

constexpr unsigned COL(const unsigned index) { return index % 8; }

// Returns true if only one bit is set in given U64. This is much faster
// than using PopCount function.
constexpr bool OnlyOneBitSet(U64 v) { return v && !(v & (v - 1)); }

constexpr char DigitToChar(const int digit) {
  return (digit >= 0 && digit <= 9) ? static_cast<char>(digit + 48)
                                    : throw std::logic_error("Expected digit");
}

constexpr bool IsOnBoard(const int row, const int col) {
  return row >= 0 && row <= 7 && col >= 0 && col <= 7;
}

constexpr U64 SetBit(int row, int col) {
  return IsOnBoard(row, col) ? (1ULL << INDX(row, col)) : 0ULL;
}

constexpr U64 SetBit(const char* sq) {
  return SetBit(CharToDigit(sq[1]) - 1, sq[0] - 'a');
}

constexpr bool IsStandard(Variant variant) {
  return variant == Variant::STANDARD;
}

constexpr bool IsAntichess(Variant variant) {
  return variant == Variant::ANTICHESS;
}

constexpr bool IsSuicide(Variant variant) {
  return variant == Variant::SUICIDE;
}

constexpr bool IsAntichessLike(Variant variant) {
  return IsAntichess(variant) || IsSuicide(variant);
}

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

constexpr Side OppositeSide(Side side) {
  return side == Side::BLACK ? Side::WHITE
                             : (side == Side::WHITE ? Side::BLACK : Side::NONE);
}

// Maps a side to an index; 0 for white and 1 for black.
constexpr int SideIndex(const Side side) { return static_cast<int>(side) - 1; }

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

std::vector<std::string> SplitString(const std::string& s, char delim);
int StringToInt(const std::string& s);
std::string LongToString(long l);
bool GlobFiles(const std::string& regex, std::vector<std::string>* filenames);

#endif
