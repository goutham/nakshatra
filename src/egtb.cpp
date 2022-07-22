#include "egtb.h"
#include "board.h"
#include "common.h"
#include "config.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

constexpr int piece_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

int ComputeBoardDescriptionId(const Board& board) {
  U64 bitboard = board.BitBoard();
  int value = 1;
  while (bitboard) {
    const int lsb_index = Lsb1(bitboard);
    value *= piece_primes[PieceIndex(board.PieceAt(lsb_index))];
    bitboard ^= (1ULL << lsb_index);
  }
  return value;
}

U64 ComputeEGTBIndex(const Board& board) {
  U64 index = 0, half_space = 1;
  int num_pieces = 0;
  for (Piece piece = -PAWN; piece <= PAWN; ++piece) {
    if (piece == NULLPIECE)
      continue;
    U64 piece_bitboard = board.BitBoard(piece);
    while (piece_bitboard) {
      const int lsb_index = Lsb1(piece_bitboard);
      index = 64 * index + lsb_index;
      half_space *= 64;
      ++num_pieces;
      piece_bitboard ^= (1ULL << lsb_index);
    }
  }
  index = SideIndex(board.SideToMove()) * half_space + index;
  return index;
}

int EGTBResult(const EGTBIndexEntry& entry) {
  if (entry.result == 1) {
    return WIN;
  } else if (entry.result == -1) {
    return -WIN;
  } else if (entry.result == 0) {
    return DRAW;
  } else {
    assert(false); // no other values currently supported.
  }
}

EGTB::EGTB(const std::vector<std::string>& egtb_files)
    : egtb_files_(egtb_files), initialized_(false), egtb_hits_(0ULL),
      egtb_misses_(0ULL) {}

void EGTB::Initialize() {
  for (const std::string& egtb_file : egtb_files_) {
    const auto parts = SplitString(egtb_file, '/');
    int board_desc_id =
        StringToInt(SplitString(parts.at(parts.size() - 1), '.').at(0));
    assert(board_desc_id != 0);
    std::ifstream ifs(egtb_file, std::ifstream::binary);
    ifs.seekg(0, std::ios_base::end);
    const int64_t file_size = ifs.tellg();
    const int num_entries = file_size / sizeof(EGTBIndexEntry);
    assert(egtb_index_.find(board_desc_id) == egtb_index_.end());
    auto& v = egtb_index_[board_desc_id];
    v.resize(num_entries);
    ifs.seekg(0, std::ios_base::beg);
    EGTBIndexEntry* tmp = new EGTBIndexEntry[num_entries];
    ifs.read(reinterpret_cast<char*>(tmp), file_size);
    for (int i = 0; i < num_entries; ++i) {
      v[i] = tmp[i];
    }
    delete[] tmp;
    ifs.close();
  }
  initialized_ = true;
}

const EGTBIndexEntry* EGTB::Lookup(const Board& board) {
  assert(initialized_);
  int board_desc_id = ComputeBoardDescriptionId(board);
  auto v = egtb_index_.find(board_desc_id);
  if (v == egtb_index_.end()) {
    return nullptr;
  }
  U64 index = ComputeEGTBIndex(board);
  if (index >= v->second.size()) {
    return nullptr;
  }
  EGTBIndexEntry& entry = v->second.at(index);
  if (!entry.next_move.is_valid()) {
    ++egtb_misses_;
    return nullptr;
  }
  ++egtb_hits_;
  return &entry;
}

void EGTB::LogStats() {
  assert(initialized_);
  std::cout << "# EGTB hits: " << egtb_hits_ << std::endl;
  std::cout << "# EGTB misses: " << egtb_misses_ << std::endl;
}

void PrintEGTBIndexEntry(const EGTBIndexEntry& entry) {
  if (!entry.next_move.is_valid()) {
    std::cout << "# EGTBIndexEntry object not valid." << std::endl;
    return;
  }
  std::cout << "# EGTB best move: " << entry.next_move.str() << std::endl;
  std::cout << "# Moves to end: " << entry.moves_to_end << std::endl;
  std::string result = "Unknown";
  if (entry.result == 1) {
    result = "Win";
  } else if (entry.result == -1) {
    result = "Loss";
  } else if (entry.result == 0) {
    result = "Draw";
  }
  std::cout << "# Result for side to move: " << result << std::endl;
}

EGTB* GetEGTB(const Variant variant) {
  if (variant == Variant::ANTICHESS) {
    static EGTB* antichess_egtb = [] {
      std::vector<std::string> egtb_filenames;
      assert(GlobFiles(ANTICHESS_EGTB_PATH_GLOB, &egtb_filenames));
      auto egtb = new EGTB(egtb_filenames);
      egtb->Initialize();
      return egtb;
    }();
    return antichess_egtb;
  }
  return nullptr;
}
