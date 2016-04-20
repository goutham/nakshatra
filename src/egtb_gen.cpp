#include "board.h"
#include "common.h"
#include "egtb.h"
#include "egtb_gen.h"
#include "eval_suicide.h"
#include "fen.h"
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

EGTBElement* EGTBStore::Get(const Board& board) {
  if (board.EnpassantTarget() != -1) return nullptr;
  int board_desc_id = ComputeBoardDescriptionId(board);
  U64 index = ComputeEGTBIndex(board);
  auto elem = store_[board_desc_id].find(index);
  if (elem == store_[board_desc_id].end()) {
    return nullptr;
  }
  return &elem->second;
}

void EGTBStore::Put(const Board& board, int moves_to_end,
                    Move next_move, Side winner) {
  EGTBElement e;
  e.fen = board.ParseIntoFEN();
  e.moves_to_end = moves_to_end;
  e.next_move = next_move;
  e.winner = winner;
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
    const string& filename = ss.str() + ".egtb";
    std::ofstream ofs(filename, std::ofstream::binary);
    for (const auto& elem2 : store_[elem.first]) {
      EGTBIndexEntry entry;
      entry.next_move = elem2.second.next_move;
      entry.moves_to_end = elem2.second.moves_to_end;
      const Side playing_side = FEN::PlayerColor(elem2.second.fen);
      entry.result = (playing_side == elem2.second.winner)
          ? 1
          : ((playing_side == OppositeSide(elem2.second.winner))
              ? -1 : 0);
      U64 index = elem2.first;
      ofs.seekp(index * sizeof(EGTBIndexEntry), std::ios_base::beg);
      ofs.write(reinterpret_cast<char*>(&entry), sizeof(EGTBIndexEntry));
    }
    ofs.close();
  }
}

void EGTBGenerate(list<string> all_pos_list, Side winning_side,
                  EGTBStore* store) {
  for (list<string>::iterator iter = all_pos_list.begin();
       iter != all_pos_list.end();) {
    Board board(Variant::SUICIDE, *iter);
    MoveGeneratorSuicide movegen(board);
    EvalSuicide eval(&board, &movegen, nullptr);
    int result = eval.Result();
    if (result == WIN) {
      store->Put(board, 0, Move(), board.SideToMove());
      iter = all_pos_list.erase(iter);
    } else if (result == -WIN) {
      store->Put(board, 0, Move(), OppositeSide(board.SideToMove()));
      iter = all_pos_list.erase(iter);
    } else if (result == DRAW) {
      iter = all_pos_list.erase(iter);
    } else {
      ++iter;
    }
  }

  int superbest = 0;
  int i = 0;
  while (true) {
    unsigned all_pos_list_size = all_pos_list.size(), progress = 0;
    printf("Size: %d, %u, %d\n", i, all_pos_list_size, superbest);
    EGTBStore temp_store;
    bool deleted = false;
    double last_percent = 0.0;
    for (list<string>::iterator iter = all_pos_list.begin();
        iter != all_pos_list.end();) {
      double percent = (static_cast<double>(progress) / all_pos_list_size) * 100;
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
      if (board.SideToMove() == winning_side) {
        int count = 0;
        int best = 10000;
        Move m;
        for (int i = 0; i < movelist.size(); ++i) {
          const Move& move = movelist.get(i);
          board.MakeMove(move);
          EGTBElement* e = store->Get(board);
          if (e != NULL &&
              e->winner == winning_side &&
              e->moves_to_end + 1 < best) {
            best = e->moves_to_end + 1;
            m = move;
            ++count;
          }
          board.UnmakeLastMove();
        }
        if (count >= 1) {
          if (best > superbest) superbest = best;
          temp_store.Put(board, best, m, winning_side);
          iter = all_pos_list.erase(iter);
          deleted = true;
        } else {
          ++iter;
        }
      } else {
        int count = 0;
        int best = -1;
        Move m;
        for (int i = 0; i < movelist.size(); ++i) {
          const Move& move = movelist.get(i);
          board.MakeMove(move);
          EGTBElement* e = store->Get(board);
          if (e != NULL &&
              e->winner == winning_side) {
            if (e->moves_to_end + 1 > best) {
              best = e->moves_to_end + 1;
              m = move;
            }
            ++count;
          }
          board.UnmakeLastMove();
        }
        if (count == movelist.size()) {
          if (best > superbest) superbest = best;
          temp_store.Put(board, best, m, winning_side);
          iter = all_pos_list.erase(iter);
          deleted = true;
        } else {
          ++iter;
        }
      }
    }
    printf("\n");
    if (!deleted) {
      break;
    }
    store->MergeFrom(temp_store);
    ++i;
  }
  printf("\n");
}
