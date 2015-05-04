#include "common.h"
#include "slider_attacks.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>

using std::vector;

template <typename T>
void ReadFromFile(const std::string& filename, vector<T>* v) {
  std::ifstream ifs(filename.c_str(), std::ios::in);
  if (!ifs.is_open()) {
    std::cerr << "Unable to open file " << filename << std::endl;
    exit(-1);
  }
  std::string s;
  while (ifs >> s) {
    v->push_back(T(strtoull(s.c_str(), nullptr, 16)));
  }
  ifs.close();
}

void SliderAttacks::Initialize() {
  vector<U64> rook_masks;
  ReadFromFile(kRookMasks, &rook_masks);
  assert(rook_masks.size() == BOARD_SIZE);

  vector<U64> bishop_masks;
  ReadFromFile(kBishopMasks, &bishop_masks);
  assert(bishop_masks.size() == BOARD_SIZE);

  vector<U64> rook_magics;
  ReadFromFile(kRookMagics, &rook_magics);
  assert(rook_magics.size() == BOARD_SIZE);

  vector<U64> bishop_magics;
  ReadFromFile(kBishopMagics, &bishop_magics);
  assert(bishop_magics.size() == BOARD_SIZE);

  vector<int> rook_offsets;
  ReadFromFile(kRookOffsets, &rook_offsets);
  assert(rook_offsets.size() == BOARD_SIZE);

  vector<int> bishop_offsets;
  ReadFromFile(kBishopOffsets, &bishop_offsets);
  assert(bishop_offsets.size() == BOARD_SIZE);

  vector<int> rook_shifts;
  ReadFromFile(kRookShifts, &rook_shifts);
  assert(rook_shifts.size() == BOARD_SIZE);

  vector<int> bishop_shifts;
  ReadFromFile(kBishopShifts, &bishop_shifts);
  assert(bishop_shifts.size() == BOARD_SIZE);

  ReadFromFile(kRookAttackTable, &rook_attack_table_);
  rook_attack_table_.shrink_to_fit();

  ReadFromFile(kBishopAttackTable, &bishop_attack_table_);
  bishop_attack_table_.shrink_to_fit();

  for (int i = 0; i < BOARD_SIZE; ++i) {
    rook_magics_[i] = { rook_masks.at(i), rook_magics.at(i),
                        rook_shifts.at(i), rook_offsets.at(i) };
    bishop_magics_[i] = { bishop_masks.at(i), bishop_magics.at(i),
                          bishop_shifts.at(i), bishop_offsets.at(i) };
  }
}
