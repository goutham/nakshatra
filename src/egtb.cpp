#include "board.h"
#include "common.h"
#include "egtb.h"

#include <fstream>
#include <iostream>
#include <string>

int64_t GetIndex_1_1(const Board& board) {
  const Side cur_side = board.SideToMove();
  const Side opp_side = OppositeSide(cur_side);
  const int cur_piece_index = log2U(board.BitBoard(cur_side));
  const int opp_piece_index = log2U(board.BitBoard(opp_side));
  Piece cur_piece = board.PieceAt(cur_piece_index);
  Piece opp_piece = board.PieceAt(opp_piece_index);
  if (cur_piece == -PAWN) {
    cur_piece = 7;
  } else {
    cur_piece = PieceType(cur_piece);
  }
  if (opp_piece == -PAWN) {
    opp_piece = 7;
  } else {
    opp_piece = PieceType(opp_piece);
  }
  return opp_piece_index +
         64 * opp_piece +
         8 * 64 * cur_piece_index +
         8 * 64 * 64 * cur_piece;
}

int EGTBResult(const EGTBIndexEntry& entry) {
  if (entry.result == 1) {
    return WIN;
  } else if (entry.result == -1) {
    return -WIN;
  } else {
    assert(false);  // no other values currently supported.
  }
}

EGTB::EGTB(const std::string& egtb_file,
           const Board& board)
    : egtb_file_(egtb_file),
      board_(board),
      initialized_(false),
      egtb_index_(nullptr),
      num_entries_(0LL),
      egtb_hits_(0ULL),
      egtb_misses_(0ULL) {}

EGTB::~EGTB() {
  if (egtb_index_) {
    delete egtb_index_;
  }
}

void EGTB::Initialize() {
  std::ifstream ifs(egtb_file_.c_str(), std::ifstream::binary);
  ifs.seekg(0, std::ios_base::end);
  int64_t file_size = ifs.tellg();
  std::cout << "# Size of " << egtb_file_ << ": " << file_size << std::endl;
  num_entries_ = file_size / sizeof(EGTBIndexEntry);
  std::cout << "# Num EGTB entries: " << num_entries_ << std::endl;
  ifs.seekg(0, std::ios_base::beg);
  egtb_index_ = new EGTBIndexEntry[num_entries_];
  ifs.read(reinterpret_cast<char*>(egtb_index_), file_size);
  ifs.close();
  initialized_ = true;
}

int64_t EGTB::GetIndex() {
  assert(initialized_);
  return ::GetIndex_1_1(board_);
}

const EGTBIndexEntry* EGTB::Lookup() {
  assert(initialized_);
  int64_t entry_index = GetIndex();
  // This can happen when a query is made for an index beyond
  // the range stored in the EGTB.
  if (entry_index >= num_entries_) {
    return nullptr;
  }
  EGTBIndexEntry* entry = egtb_index_ + entry_index;
  if (!entry->next_move.is_valid()) {
    ++egtb_misses_;
    return nullptr;
  }
  ++egtb_hits_;
  return entry;
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
  std::cout << "# Result for side to move: "
      << (entry.result == 1 ? "Win" : (entry.result == -1 ? "Loss" : "Unknown"))
      << std::endl;
}
