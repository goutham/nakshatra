#ifndef COMMON_H
#define COMMON_H

#include <climits>
#include <cstdint>
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

enum class Side {
  NONE,
  WHITE,
  BLACK
};

enum NodeType { FAIL_HIGH_NODE, FAIL_LOW_NODE, EXACT_NODE };

enum Variant { NORMAL, SUICIDE };

// Uses de Bruijn Sequences to Index a 1 in a Computer Word.
int log2U(U64 bb);

// Number of set bits in a U64 integer.
unsigned PopCount(U64 x);

constexpr unsigned INDX(const unsigned row, const unsigned col) {
  return row * 8 + col;
}

constexpr unsigned ROW(const unsigned index) {
  return index / 8;
}

constexpr unsigned COL(const unsigned index) {
  return index % 8;
}

constexpr Side OppositeSide(Side side) {
  return side == Side::BLACK ? Side::WHITE
                             : (side == Side::WHITE ? Side::BLACK : Side::NONE);
}

// Maps a side to an index; 0 for white and 1 for black.
constexpr int SideIndex(const Side side) {
  return side == Side::WHITE ? 0 : 1;
}

// Returns true if only one bit is set in given U64. This is much faster
// than using PopCount function.
constexpr bool OnlyOneBitSet(U64 v) {
  return v && !(v & (v - 1));
}

constexpr char DigitToChar(const int digit) {
  return (digit >= 0 && digit <= 9) ? static_cast<char>(digit + 48)
                                    : throw std::logic_error("Expected digit");
}

constexpr int CharToDigit(const char c) {
  return (c >= '0' && c <= '9')
             ? (c - 48)
             : throw std::logic_error("Expected char between '0' and '9'");
}

std::vector<std::string> SplitString(const std::string& s, char delim);
int StringToInt(const std::string& s);
std::string LongToString(long l);

static const char kRookMagics[] = "rook_magics.magic";
static const char kRookMasks[] = "rook_masks.magic";
static const char kRookShifts[] = "rook_shifts.magic";
static const char kRookOffsets[] = "rook_offsets.magic";
static const char kRookAttackTable[] = "rook_attack_table.magic";

static const char kBishopMagics[] = "bishop_magics.magic";
static const char kBishopMasks[] = "bishop_masks.magic";
static const char kBishopShifts[] = "bishop_shifts.magic";
static const char kBishopOffsets[] = "bishop_offsets.magic";
static const char kBishopAttackTable[] = "bishop_attack_table.magic";

#endif
