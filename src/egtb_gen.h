#ifndef EGTB_GEN_H
#define EGTB_GEN_H

#include "piece.h"

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
  EGTBElement* Get(std::string fen);

  void Put(std::string fen, int moves_to_end, Move next_move, Side winner);

  const std::unordered_map<std::string, EGTBElement>& GetMap() {
    return store_;
  }

  void MergeFrom(EGTBStore store);

  void Write(std::ofstream& ofs);

 private:
  std::unordered_map<std::string, EGTBElement> store_;
};

void EGTBGenerate(std::list<std::string> positions,
                  Side winning_side, EGTBStore* store);

#endif
