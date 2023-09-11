#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

void EGTBGenerate(std::list<std::string> all_pos_list, EGTBStore* store) {
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
  while (true) {
    unsigned all_pos_list_size = all_pos_list.size(), progress = 0;
    printf("Size: %d, %u, %d\n", s, all_pos_list_size, superbest);
    EGTBStore temp_store;
    bool deleted = false;
    double last_percent = 0.0;
    for (auto iter = all_pos_list.begin(); iter != all_pos_list.end();) {
      double percent =
          (static_cast<double>(progress) / all_pos_list_size) * 100;
      if (percent >= last_percent + 1 || percent + 0.1 >= 100) {
        printf("%5.2lf %%\r", percent);
        fflush(stdout);
        last_percent = percent;
      }
      ++progress;
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
    printf("\n");
    if (!deleted)
      break;
    store->MergeFrom(temp_store);
    ++s;
  }
  printf("\n");
}

void GeneratePermutations(const std::vector<Piece>& pieces,
                          const std::vector<Side>& next_player_sides,
                          size_t start_index, Board* board,
                          std::set<std::string>* permutations) {
  if (start_index == pieces.size()) {
    for (const Side side : next_player_sides) {
      board->SetPlayerColor(side);
      permutations->insert(board->ParseIntoFEN());
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

void All1p(std::set<std::string>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    GeneratePermutations({piece}, {Side::BLACK}, 0, &board, positions);
    GeneratePermutations({-piece}, {Side::WHITE}, 0, &board, positions);
  }
}

void All2p(std::set<std::string>* positions) {
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

void All3p(std::set<std::string>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    for (Piece piece2 = KING; piece2 <= PAWN; ++piece2) {
      for (Piece piece3 = KING; piece3 <= PAWN; ++piece3) {
        GeneratePermutations({piece, piece2, piece3}, {Side::BLACK}, 0, &board,
                             positions);
        GeneratePermutations({-piece, -piece2, -piece3}, {Side::WHITE}, 0,
                             &board, positions);
        GeneratePermutations({piece, piece2, -piece3},
                             {Side::BLACK, Side::WHITE}, 0, &board, positions);
        GeneratePermutations({-piece, -piece2, piece3},
                             {Side::BLACK, Side::WHITE}, 0, &board, positions);
      }
    }
  }
}

int main() {
  std::set<std::string> positions;
  All1p(&positions);
  All2p(&positions);
  // All3p(&positions);
  std::cout << "Number of positions: " << positions.size() << std::endl;
  auto positions_list =
      std::list<std::string>(positions.begin(), positions.end());
  positions.clear();
  std::cout << "Generating EGTB..." << std::endl;
  EGTBStore store;
  EGTBGenerate(positions_list, &store);
  std::cout << "Writing out..." << std::endl;
  store.Write();
  std::cout << "Done." << std::endl;
  return 0;
}
