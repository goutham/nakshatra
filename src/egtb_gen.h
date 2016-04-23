#ifndef EGTB_GEN_H
#define EGTB_GEN_H

#include "board.h"
#include "common.h"
#include "egtb.h"
#include "piece.h"

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

class EGTBStore {
 public:
  EGTBIndexEntry* Get(const Board& board);

  void Put(const Board& board, int moves_to_end, Move next_move,
           int8_t result);

  void MergeFrom(EGTBStore store);

  const std::unordered_map<int, std::unordered_map<uint64_t, EGTBIndexEntry>>&
  GetMap() {
    return store_;
  }

  void Write();

 private:
  std::unordered_map<int, std::unordered_map<uint64_t, EGTBIndexEntry>> store_;
};

void EGTBGenerate(std::list<std::string> positions, EGTBStore* store);

#endif
