#ifndef EGTB_GEN_H
#define EGTB_GEN_H

#include "board.h"
#include "common.h"
#include "piece.h"

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

struct EGTBElement {
  std::string fen;
  int moves_to_end;
  Move next_move;
  Side winner;
};

class EGTBStore {
 public:
  EGTBElement* Get(const Board& board);

  void Put(const Board& board, int moves_to_end, Move next_move,
           Side winner);

  void MergeFrom(EGTBStore store);

  const std::unordered_map<int, std::unordered_map<uint64_t, EGTBElement>>&
  GetMap() {
    return store_;
  }

  void Write(std::ofstream& ofs);

 private:
  std::unordered_map<int, std::unordered_map<uint64_t, EGTBElement>> store_;
};

void EGTBGenerate(std::list<std::string> positions,
                  Side winning_side, EGTBStore* store);

#endif
