#include "attacks.h"
#include "common.h"
#include "side_relative.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <tuple>

using std::vector;

namespace {

class Direction {
public:
  enum D {
    NORTH,
    SOUTH,
    EAST,
    WEST,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH_EAST,
    SOUTH_WEST
  };

  Direction(D direction) : direction_(direction) {}

  // Index of the next square along this direction. Returns -1 if
  // next index is outside the board.
  int NextIndex(int index) const {
    int row = ROW(index);
    int col = COL(index);

    switch (direction_) {
    case NORTH:
      ++row;
      break;
    case SOUTH:
      --row;
      break;
    case EAST:
      ++col;
      break;
    case WEST:
      --col;
      break;
    case NORTH_EAST:
      ++row;
      ++col;
      break;
    case NORTH_WEST:
      ++row;
      --col;
      break;
    case SOUTH_EAST:
      --row;
      ++col;
      break;
    case SOUTH_WEST:
      --row;
      --col;
      break;
    }
    return (row > 7 || col > 7 || row < 0 || col < 0) ? -1 : INDX(row, col);
  }

  // Number of squares from given square to the edge of the board
  // along this direction.
  int EdgeDistance(int index) const {
    int row = ROW(index);
    int col = COL(index);

    auto inv = [](int x) -> int { return 7 - x; };

    int d = -1;
    switch (direction_) {
    case NORTH:
      d = inv(row);
      break;
    case SOUTH:
      d = row;
      break;
    case EAST:
      d = inv(col);
      break;
    case WEST:
      d = col;
      break;
    case NORTH_EAST:
      d = std::min(inv(row), inv(col));
      break;
    case NORTH_WEST:
      d = std::min(inv(row), col);
      break;
    case SOUTH_EAST:
      d = std::min(row, inv(col));
      break;
    case SOUTH_WEST:
      d = std::min(row, col);
      break;
    }
    assert(d >= 0 && d <= 7);
    return d;
  }

private:
  D direction_;
};

U64 MaskBits(const Direction& direction, const int index) {
  U64 bitboard = 0ULL;
  int next_index = index;
  // Exclude source bit and the edge of board in given direction.
  while ((next_index = direction.NextIndex(next_index)) >= 0 &&
         direction.NextIndex(next_index) >= 0) {
    bitboard |= (1ULL << next_index);
  }
  return bitboard;
}

void GenerateOccupancies(const Direction& direction, const int index,
                         vector<U64>* bbv) {
  // Number of squares in this direction excluding current square and
  // edge of the board.
  const int num_squares = direction.EdgeDistance(index) - 1;
  if (num_squares <= 0) {
    return;
  }

  // Number of possible piece occupancies in these squares along
  // the given direction.
  const unsigned num_occupancies = (1U << num_squares);

  // Create bitboard for each occupancy with the index next to given
  // index as starting point, along the given direction.
  for (unsigned occupancy = 0U; occupancy < num_occupancies; ++occupancy) {
    U64 bitboard = 0ULL;
    int next_index = index;
    for (unsigned bit_mask = 1U; bit_mask <= occupancy; bit_mask <<= 1) {
      next_index = direction.NextIndex(next_index);
      assert(next_index != -1);
      bitboard |= (U64(!!(occupancy & bit_mask)) << next_index);
    }
    bbv->push_back(bitboard);
  }
}

class OccupancyCombiner {
public:
  OccupancyCombiner(int index) : index_(index) {}

  // Combines occupancy bitboards by bitwise ORing each stored
  // bitboard with bitboards generated by GenerateOccupancies
  // along given direction.
  void Combine(const Direction& direction);

  const vector<U64>& Occupancies() const { return occupancies_; }

private:
  const int index_;
  vector<U64> occupancies_;
};

void OccupancyCombiner::Combine(const Direction& direction) {
  vector<U64> bbv;
  GenerateOccupancies(direction, index_, &bbv);
  if (bbv.empty()) {
    return;
  }
  if (occupancies_.empty()) {
    occupancies_.insert(occupancies_.end(), bbv.begin(), bbv.end());
    return;
  }
  vector<U64> tmp;
  for (const U64 bb : bbv) {
    for (const U64 occupancy : occupancies_) {
      tmp.push_back(bb | occupancy);
    }
  }
  occupancies_.swap(tmp);
}

// Generate an attack bitboard from a given square in the given direction
// for a specific occupancy of pieces.
U64 GenerateAttack(const Direction& direction, const int index,
                   const U64 occupancy) {
  U64 attack_bb = 0ULL;
  for (int i = index; (i = direction.NextIndex(i)) != -1;) {
    attack_bb |= (1ULL << i);
    if (occupancy & (1ULL << i)) {
      break;
    }
  }
  return attack_bb;
}

void GenerateAttackTable(const vector<Direction>& directions, const int index,
                         const int shift_bits, const U64 magic,
                         vector<U64>* attack_table) {
  // Generate occupancies.
  OccupancyCombiner combiner(index);
  for (const Direction& direction : directions) {
    combiner.Combine(direction);
  }
  vector<U64> occupancies = combiner.Occupancies();

  // Generate attacks.
  vector<U64> attacks;
  for (const U64 occupancy : occupancies) {
    U64 attack = 0ULL;
    for (const Direction& direction : directions) {
      attack |= GenerateAttack(direction, index, occupancy);
    }
    attacks.push_back(attack);
  }

  // No bishop or rook attack can cover all squares of the board.
  static const U64 kInvalidAttack = ~0ULL;

  vector<U64> table(1U << shift_bits, kInvalidAttack);
  for (size_t k = 0; k < occupancies.size(); ++k) {
    const U64 occupancy = occupancies.at(k);
    const U64 attack = attacks.at(k);
    const int offset = (occupancy * magic) >> (64 - shift_bits);
    if (table.at(offset) == kInvalidAttack || table.at(offset) == attack) {
      table.at(offset) = attack;
    } else {
      throw std::runtime_error("Collision occurred");
    }
  }

  attack_table->swap(table);
}

using Masks64 = std::array<U64, 64>;
using Offsets64 = std::array<U64, 64>;
using AttackTable = vector<U64>;

std::tuple<Masks64, Offsets64, AttackTable>
Generate(const vector<Direction>& directions, const int shifts[],
         const U64 magics[]) {
  Masks64 masks;
  Offsets64 offsets;
  AttackTable attack_table;

  for (int i = 0; i < 64; ++i) {
    masks.at(i) = 0ULL;
    for (const Direction& d : directions) {
      masks.at(i) |= MaskBits(d, i);
    }

    vector<U64> tmp_attack_table;
    GenerateAttackTable(directions, i, shifts[i], magics[i], &tmp_attack_table);
    offsets.at(i) = attack_table.size();
    attack_table.insert(attack_table.end(), tmp_attack_table.begin(),
                        tmp_attack_table.end());
  }

  return std::make_tuple(masks, offsets, attack_table);
}

// clang-format off
// Pre-computed using magic-bits library.
constexpr U64 rook_magics[] = {
    0x880081080c00020,  0x210020c000308100,
    0x80082001100280,   0x1001000a0050108,
    0x200041029600a00,  0x5100010008220400,
    0x8280120001000d80, 0x1880012100014080,
    0x3040800340008020, 0x400400050026003,
    0x21002000104902,   0x20900200a100100,
    0xd800802840080,    0x2808004000600,
    0x24001002110814,   0x2000800541000480,
    0x8000ee8002400080, 0x24c04010002005,
    0x822002401000c800, 0x2040808010000800,
    0x804080800c000802, 0x2a0080110402004,
    0x201044000810010a, 0x4080020004004483,
    0x4d84400180228000, 0x1406400880200880,
    0x801200402203,     0x1080080280100084,
    0x402140080080080,  0xa880c0080020080,
    0x342000200080405,  0x20004a8200050044,
    0x8280c00020800889, 0x8002201000400940,
    0x44a200101001542,  0x88090021005000,
    0x3008004200c00400, 0x284120080800400,
    0x4462106804000201, 0x1008240382000061,
    0x80400080208002,   0x20100040004020,
    0x4000802042020010, 0x40a002042120008,
    0x12a008820120004,  0x6000408020010,
    0x2008405020008,    0x80100c0040820003,
    0x2800100446100,    0xa0982002400080,
    0x9a0080010014040,  0x380c209200420a00,
    0xc04008108000580,  0xc002008004002280,
    0x2900842a000100,   0x40100008a004300,
    0x10211800020c3,    0xa08412050242,
    0x2001004010200489, 0xa00081000210045,
    0x4512002810204402, 0x8c22000401102802,
    0x485000082005401,  0x100208400ce,
};

// Pre-computed using magic-bits library.
constexpr U64 bishop_magics[] = {
    0x820420460402a080,  0x20021200451400,
    0x10011200218000,    0x4040888100800,
    0x6211001000400,     0x401042240021400,
    0x884029888090060,   0x24202808080810,
    0x20242038024080,    0x80021081010102,
    0x100004090c030120,  0x210c0420814205,
    0x408311040061010,   0x4900011016100900,
    0x6841020d30461020,  0x220112088080800,
    0x8040000802080628,  0x4a48000408480040,
    0x2010000e00b20060,  0x1004020809409102,
    0x1011090400801,     0x2002000420842000,
    0xa01200443a090402,  0x1010082a4020221,
    0x7118c00204100682,  0x2223440021040c00,
    0xa208018c08020142,  0x4404004010200,
    0x14840004802000,    0x204016024100401,
    0x23021a0005451020,  0x204222022c10410,
    0x122010002002b0,    0x2501000022200,
    0x84002804001800a1,  0x1002080800060a00,
    0x40018020120220,    0x41108881004a0100,
    0x800c041410224502,  0x4001020080006403,
    0x205091140081002,   0x491210901c001808,
    0x400084048001000,   0x8824200910800,
    0xca00400408228102,  0x2042240800221200,
    0x54082081000405,    0x1010202004291,
    0x4040a40920100100,  0x4802060101082c10,
    0x208002623100105,   0x1000e2c084040010,
    0x202302400682008a,  0x20820c50024a0c10,
    0x200c20020c090100,  0x684010822028800,
    0x400e002101482012,  0x800804218044242,
    0x8a0040201008820,   0xc000000024420200,
    0x3404102090c20200,  0x8000840810104981,
    0x80330810d0009101,  0x4011001020084,
};

constexpr int rook_shifts[] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};

constexpr int bishop_shifts[] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};
// clang-format on

const auto [rook_masks, rook_offsets, rook_attack_table] =
    Generate({Direction(Direction::NORTH), Direction(Direction::SOUTH),
              Direction(Direction::EAST), Direction(Direction::WEST)},
             rook_shifts, rook_magics);

const auto [bishop_masks, bishop_offsets, bishop_attack_table] = Generate(
    {Direction(Direction::NORTH_EAST), Direction(Direction::NORTH_WEST),
     Direction(Direction::SOUTH_EAST), Direction(Direction::SOUTH_WEST)},
    bishop_shifts, bishop_magics);

// KING and KNIGHT attacks

const auto knight_attacks = []() {
  std::array<U64, 64> knight_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      knight_attacks[INDX(i, j)] =
          (SetBit(i - 1, j - 2) | SetBit(i + 1, j - 2) | SetBit(i + 2, j - 1) |
           SetBit(i + 2, j + 1) | SetBit(i + 1, j + 2) | SetBit(i - 1, j + 2) |
           SetBit(i - 2, j + 1) | SetBit(i - 2, j - 1));
    }
  }
  return knight_attacks;
}();

const auto king_attacks = []() {
  std::array<U64, 64> king_attacks;
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      king_attacks[INDX(i, j)] =
          (SetBit(i - 1, j - 1) | SetBit(i, j - 1) | SetBit(i + 1, j - 1) |
           SetBit(i + 1, j) | SetBit(i + 1, j + 1) | SetBit(i, j + 1) |
           SetBit(i - 1, j + 1) | SetBit(i - 1, j));
    }
  }
  return king_attacks;
}();

U64 RookAttacks(const U64 bitboard, const int index) {
  const U64 occupancy = bitboard & rook_masks[index];
  const int attack_table_index =
      ((occupancy * rook_magics[index]) >> (64 - rook_shifts[index])) +
      rook_offsets[index];
  return rook_attack_table[attack_table_index];
}

U64 BishopAttacks(const U64 bitboard, const int index) {
  const U64 occupancy = bitboard & bishop_masks[index];
  const int attack_table_index =
      ((occupancy * bishop_magics[index]) >> (64 - bishop_shifts[index])) +
      bishop_offsets[index];
  return bishop_attack_table[attack_table_index];
}

U64 QueenAttacks(const U64 bitboard, const int index) {
  return RookAttacks(bitboard, index) | BishopAttacks(bitboard, index);
}

U64 KnightAttacks(const U64 unused_bitboard, const int index) {
  return knight_attacks[index];
}

U64 KingAttacks(const U64 unused_bitboard, const int index) {
  return king_attacks[index];
}

using AttacksFn = U64 (*)(const U64, const int);

AttacksFn attacks_fns[6] = {nullptr,     KingAttacks,   QueenAttacks,
                            RookAttacks, BishopAttacks, KnightAttacks};

} // namespace

namespace attacks {

U64 Attacks(const U64 bitboard, const int index, const Piece piece) {
  return attacks_fns[PieceType(piece)](bitboard, index);
}

U64 SquareAttackers(const int square, const Piece attacking_piece,
                    const U64 occ, const U64 attacking_side_piece_occ) {
  const Side attacking_side = PieceSide(attacking_piece);
  const U64 bb = (1ULL << square);
  U64 attack_bb = 0ULL;
  if (PieceType(attacking_piece) == PAWN) {
    switch (attacking_side) {
    case Side::WHITE:
      attack_bb = side_relative::PushNorthEast<Side::BLACK>(bb) |
                  side_relative::PushNorthWest<Side::BLACK>(bb);
      break;
    case Side::BLACK:
      attack_bb = side_relative::PushNorthEast<Side::WHITE>(bb) |
                  side_relative::PushNorthWest<Side::WHITE>(bb);
      break;
    default:
      throw std::logic_error("cannot process side");
    }
  } else {
    attack_bb = Attacks(occ, square, attacking_piece);
  }
  return attack_bb & attacking_side_piece_occ;
}

} // namespace attacks
