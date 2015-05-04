#ifndef EGTB_GEN_H
#define EGTB_GEN_H

#include "piece.h"

#include <map>
#include <string>
#include <vector>

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

  const std::map<std::string, EGTBElement>& GetMap() {
    return store_;
  }

  void MergeFrom(EGTBStore store);

  void Write(std::ofstream& ofs);

 private:
  std::map<std::string, EGTBElement> store_;
};

class EGTBGenerator {
 public:
  void Generate(std::vector<std::string> final_pos_list,
                std::vector<std::string> all_pos_list,
                Side winning_side,
                EGTBStore* store);

  void Generate(std::vector<std::string> all_pos_list,
                Side winning_side,
                EGTBStore* store);
};


#endif
