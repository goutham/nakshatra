#include "egtb.h"
#include "board.h"
#include "common.h"
#include "egtb_gen.h"
#include "eval_suicide.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "piece.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#define MAX 10000

using std::list;
using std::string;

EGTBIndexEntry* EGTBStore::Get(const Board& board) {
  if (board.EnpassantTarget() != -1)
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

void EGTBStore::Put(const Board& board, int moves_to_end, Move next_move,
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

void EGTBStore::MergeFrom(EGTBStore store) {
  const auto& egtb_map = store.GetMap();
  for (const auto& elem : egtb_map) {
    for (const auto& elem2 : elem.second) {
      assert(store_[elem.first].find(elem2.first) == store_[elem.first].end());
      store_[elem.first][elem2.first] = elem2.second;
    }
  }
}

void EGTBStore::Write() {
  for (const auto& elem : store_) {
    std::stringstream ss;
    ss << elem.first;
    const string& filename = "egtb/" + ss.str() + ".egtb";
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

void EGTBGenerate(list<string> all_pos_list, EGTBStore* store) {
  for (list<string>::iterator iter = all_pos_list.begin();
       iter != all_pos_list.end();) {
    Board board(Variant::SUICIDE, *iter);
    MoveGeneratorSuicide movegen(board);
    EvalSuicide eval(&board, &movegen, nullptr);
    int result = eval.Result();
    if (result == WIN) {
      store->Put(board, 0, Move(), 1);
      iter = all_pos_list.erase(iter);
    } else if (result == -WIN) {
      store->Put(board, 0, Move(), -1);
      iter = all_pos_list.erase(iter);
    } else if (result == DRAW) {
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
    for (list<string>::iterator iter = all_pos_list.begin();
         iter != all_pos_list.end();) {
      double percent =
          (static_cast<double>(progress) / all_pos_list_size) * 100;
      if (percent >= last_percent + 1 || percent + 0.1 >= 100) {
        printf("%5.2lf %%\r", percent);
        fflush(stdout);
        last_percent = percent;
      }
      fflush(stdout);
      ++progress;
      Board board(Variant::SUICIDE, *iter);
      MoveGeneratorSuicide movegen(board);
      MoveArray movelist;
      movegen.GenerateMoves(&movelist);
      int count_winning = 0;
      int count_losing = 0;
      int best_winning = 10000;
      int best_losing = -1;
      Move best_winning_move;
      Move best_losing_move;
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
