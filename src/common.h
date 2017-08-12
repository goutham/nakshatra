#ifndef COMMON_H
#define COMMON_H

#include <climits>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#define BOARD_SIZE 64

#define MAX_DEPTH 30

#define INF SHRT_MAX
#define WIN (SHRT_MAX - 1)
#define DRAW 0
#define UNKNOWN -1

typedef uint64_t U64;
extern std::ostream nullstream;

enum class Variant { NORMAL, SUICIDE };

enum class Side { NONE, WHITE, BLACK };

// Column numbers of files.
constexpr int FILE_A = 0;
constexpr int FILE_B = 1;
constexpr int FILE_C = 2;
constexpr int FILE_D = 3;
constexpr int FILE_E = 4;
constexpr int FILE_F = 5;
constexpr int FILE_G = 6;
constexpr int FILE_H = 7;

enum NodeType { FAIL_HIGH_NODE, FAIL_LOW_NODE, EXACT_NODE };

// Uses de Bruijn Sequences to Index a 1 in a Computer Word.
int log2U(U64 bb);

inline int Lsb1(const U64 v) { return log2U(v); }

// Number of set bits in a U64 integer.
unsigned PopCount(U64 x);

constexpr unsigned INDX(const unsigned row, const unsigned col) {
  return row * 8 + col;
}

constexpr unsigned ROW(const unsigned index) { return index / 8; }

constexpr unsigned COL(const unsigned index) { return index % 8; }

constexpr Side OppositeSide(Side side) {
  return side == Side::BLACK ? Side::WHITE
                             : (side == Side::WHITE ? Side::BLACK : Side::NONE);
}

// Maps a side to an index; 0 for white and 1 for black.
constexpr int SideIndex(const Side side) { return side == Side::WHITE ? 0 : 1; }

// Returns true if only one bit is set in given U64. This is much faster
// than using PopCount function.
constexpr bool OnlyOneBitSet(U64 v) { return v && !(v & (v - 1)); }

constexpr char DigitToChar(const int digit) {
  return (digit >= 0 && digit <= 9) ? static_cast<char>(digit + 48)
                                    : throw std::logic_error("Expected digit");
}

constexpr int CharToDigit(const char c) {
  return (c >= '0' && c <= '9')
             ? (c - 48)
             : throw std::logic_error("Expected char between '0' and '9'");
}

constexpr bool IsOnBoard(const int row, const int col) {
  return row >= 0 && row <= 7 && col >= 0 && col <= 7;
}

constexpr U64 SetBit(int row, int col) {
  return IsOnBoard(row, col) ? (1ULL << INDX(row, col)) : 0ULL;
}

std::vector<std::string> SplitString(const std::string& s, char delim);
int StringToInt(const std::string& s);
std::string LongToString(long l);
bool GlobFiles(const std::string& regex, std::vector<std::string>* filenames);

#endif
