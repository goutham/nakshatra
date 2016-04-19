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

#define MAX 10000

using std::list;
using std::string;

constexpr int piece_primes[] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37
};

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
    if (piece == NULLPIECE) continue;
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

void EGTBStore::Write(std::ofstream& ofs) {
  for (const auto& elem : store_) {
    for (const auto& elem2 : store_[elem.first]) {
      string s = elem2.second.next_move.str();
      if (s == "--") {
        s = "LOST";
      }
      ofs << elem2.second.fen << '|' << s << '|' << elem2.second.moves_to_end << '|';
      if (elem2.second.winner == Side::WHITE) {
        ofs << 'W';
      } else if (elem2.second.winner == Side::BLACK) {
        ofs << 'B';
      } else {
        ofs << 'N';
      }
      ofs << std::endl;
    }
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
