#include "egtb.h"
#include "board.h"
#include "common.h"
#include "compact.h"
#include "eval.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

class EGTBStore {
public:
  EGTBIndexEntry* Get(const Board& board) {
    if (board.EnpassantTarget() != NO_EP)
      return nullptr;
    int board_desc_id = ComputeBoardDescriptionId(board);
    if (store_.find(board_desc_id) == store_.end()) {
      return nullptr;
    }
    U64 index = ComputeEGTBIndex(board);
    auto elem = store_[board_desc_id].find(index);
    if (elem == store_[board_desc_id].end()) {
      return nullptr;
    }
    return &elem->second;
  }

  void Put(const Board& board, int moves_to_end, Move next_move,
           int8_t result) {
    EGTBIndexEntry e;
    e.moves_to_end = moves_to_end;
    e.next_move = next_move;
    e.result = result;
    int board_desc_id = ComputeBoardDescriptionId(board);
    U64 index = ComputeEGTBIndex(board);
    assert(store_[board_desc_id].find(index) == store_[board_desc_id].end());
    store_[board_desc_id][index] = e;
  }

  void MergeFrom(EGTBStore store) {
    const auto& egtb_map = store.GetMap();
    for (const auto& elem : egtb_map) {
      for (const auto& elem2 : elem.second) {
        assert(store_[elem.first].find(elem2.first) ==
               store_[elem.first].end());
        store_[elem.first][elem2.first] = elem2.second;
      }
    }
  }

  const std::unordered_map<int, std::unordered_map<uint64_t, EGTBIndexEntry>>&
  GetMap() {
    return store_;
  }

  void Write() {
    for (const auto& elem : store_) {
      std::stringstream ss;
      ss << elem.first;
      const std::string& filename = "egtb/" + ss.str() + ".egtb";
      std::ofstream ofs(filename, std::ofstream::binary);
      for (const auto& elem2 : store_[elem.first]) {
        U64 index = elem2.first;
        ofs.seekp(index * sizeof(EGTBIndexEntry), std::ios_base::beg);
        ofs.write(reinterpret_cast<const char*>(&elem2.second),
                  sizeof(EGTBIndexEntry));
      }
      ofs.close();
    }
  }

private:
  std::unordered_map<int, std::unordered_map<uint64_t, EGTBIndexEntry>> store_;
};

void EGTBGenerate(std::list<BoardDesc> all_pos_list, EGTBStore* store) {
  const size_t init_pos = all_pos_list.size();
  for (auto iter = all_pos_list.begin(); iter != all_pos_list.end();) {
    Board board(Variant::ANTICHESS, *iter);
    int result = EvalResult<Variant::ANTICHESS>(&board);
    if (result == WIN) {
      store->Put(board, 0, Move(), 1);
      iter = all_pos_list.erase(iter);
    } else if (result == -WIN) {
      store->Put(board, 0, Move(), -1);
      iter = all_pos_list.erase(iter);
    } else if (result == DRAW) {
      store->Put(board, 0, Move(), 0);
      iter = all_pos_list.erase(iter);
    } else {
      ++iter;
    }
  }

  int superbest = 0;
  int s = 0;
  for (;; ++s) {
    EGTBStore temp_store;
    bool deleted = false;
    for (auto iter = all_pos_list.begin(); iter != all_pos_list.end();) {
      Board board(Variant::ANTICHESS, *iter);
      MoveArray movelist;
      GenerateMoves<Variant::ANTICHESS>(&board, &movelist);
      int count_winning = 0;
      int count_losing = 0;
      int count_draw = 0;
      int best_winning = 10000;
      int best_losing = -1;
      int best_draw = 10000;
      Move best_winning_move;
      Move best_losing_move;
      Move best_draw_move;
      for (size_t i = 0; i < movelist.size(); ++i) {
        const Move& move = movelist.get(i);
        board.MakeMove(move);
        EGTBIndexEntry* e = store->Get(board);
        if (e != nullptr) {
          if (e->result == -1) {
            if (e->moves_to_end + 1 < best_winning) {
              best_winning = e->moves_to_end + 1;
              best_winning_move = move;
            }
            ++count_winning;
          } else if (e->result == 1) {
            if (e->moves_to_end + 1 > best_losing) {
              best_losing = e->moves_to_end + 1;
              best_losing_move = move;
            }
            ++count_losing;
          } else if (e->result == 0) {
            if (e->moves_to_end + 1 < best_draw) {
              best_draw = e->moves_to_end + 1;
              best_draw_move = move;
            }
            ++count_draw;
          } else {
            assert(false);
          }
        }
        board.UnmakeLastMove();
      }
      if (count_winning >= 1) {
        if (best_winning > superbest)
          superbest = best_winning;
        temp_store.Put(board, best_winning, best_winning_move, 1);
        iter = all_pos_list.erase(iter);
        deleted = true;
      } else if (count_losing == int(movelist.size())) {
        if (best_losing > superbest)
          superbest = best_losing;
        temp_store.Put(board, best_losing, best_losing_move, -1);
        iter = all_pos_list.erase(iter);
        deleted = true;
      } else if (count_draw >= 1 &&
                 (count_losing + count_draw == int(movelist.size()))) {
        if (best_draw > superbest)
          superbest = best_draw;
        temp_store.Put(board, best_draw, best_draw_move, 0);
        iter = all_pos_list.erase(iter);
        deleted = true;
      } else {
        ++iter;
      }
    }
    if (!deleted)
      break;
    store->MergeFrom(temp_store);
  }
  std::cout << "# [EGTB gen] debug info: #iters: " << s
            << ", max moves_to_end: " << superbest
            << ", inserted: " << init_pos - all_pos_list.size()
            << ", discarded: " << all_pos_list.size() << std::endl;
}

void GeneratePermutations(const std::vector<Piece>& pieces,
                          const std::vector<Side>& next_player_sides,
                          size_t start_index, Board* board,
                          std::set<BoardDesc>* permutations) {
  if (start_index == pieces.size()) {
    for (const Side side : next_player_sides) {
      board->SetPlayerColor(side);
      permutations->insert(board->ToCompactBoardDesc());
    }
    return;
  }
  Piece piece = pieces.at(start_index);
  for (int i = 0; i < 8; i++) {
    if (PieceType(piece) == PAWN && (i == 0 || i == 7)) {
      continue;
    }
    for (int j = 0; j < 8; j++) {
      if (board->PieceAt(i, j) == NULLPIECE) {
        board->SetPiece(INDX(i, j), piece);
        GeneratePermutations(pieces, next_player_sides, start_index + 1, board,
                             permutations);
        board->SetPiece(INDX(i, j), NULLPIECE);
      }
    }
  }
}

void All1p(std::set<BoardDesc>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    GeneratePermutations({piece}, {Side::BLACK}, 0, &board, positions);
    GeneratePermutations({-piece}, {Side::WHITE}, 0, &board, positions);
  }
}

void All2p(std::set<BoardDesc>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    for (Piece piece2 = KING; piece2 <= PAWN; ++piece2) {
      GeneratePermutations({piece, piece2}, {Side::BLACK}, 0, &board,
                           positions);
      GeneratePermutations({-piece, -piece2}, {Side::WHITE}, 0, &board,
                           positions);
      GeneratePermutations({piece, -piece2}, {Side::BLACK, Side::WHITE}, 0,
                           &board, positions);
    }
  }
}

std::unique_ptr<EGTBStore> Generate() {
  std::set<BoardDesc> positions;
  All1p(&positions);
  All2p(&positions);
  auto positions_list =
      std::list<BoardDesc>(positions.begin(), positions.end());
  positions.clear();
  std::cout << "# [EGTB gen] generating Antichess 2 piece EGTB..." << std::endl;
  auto store = std::make_unique<EGTBStore>();
  EGTBGenerate(positions_list, store.get());
  return store;
}

void EGTB::Initialize() {
  if (initialized_) {
    return;
  }
  auto egtb_store = Generate();
  for (const auto& elem : egtb_store->GetMap()) {
    const int board_desc_id = elem.first;
    const auto& store_map = elem.second;
    assert(egtb_index_.find(board_desc_id) == egtb_index_.end());
    uint64_t max_index_val = 0;
    for (const auto& elem2 : store_map) {
      if (elem2.first > max_index_val) {
        max_index_val = elem2.first;
      }
    }
    auto& v = egtb_index_[board_desc_id];
    v.resize(static_cast<size_t>(max_index_val) + 1);
    for (const auto& elem2 : store_map) {
      v[elem2.first] = elem2.second;
    }
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
  if (IsAntichessLike(variant)) {
    static EGTB* antichess_egtb = [] {
      auto egtb = new EGTB();
      egtb->Initialize();
      return egtb;
    }();
    return antichess_egtb;
  }
  return nullptr;
}
