#ifndef TRANSPOS_H
#define TRANSPOS_H

#include "common.h"
#include "move.h"
#include "zobrist.h"

#include <cstdio>

namespace search {

struct TranspositionTableEntry {
  TranspositionTableEntry() : valid(false) {}
  bool valid;
  short int score;
  NodeType node_type;
  unsigned char depth;  // depth to the bottom of the tree in depth-restricted search.
  Move best_move;
  U64 zkey;
};

struct TranspositionTable2Entry {
  TranspositionTableEntry t1;  // depth preferred
  TranspositionTableEntry t2;  // always replace
};

class TranspositionTable {
 public:
  TranspositionTable(int size);
  ~TranspositionTable();
  TranspositionTableEntry* Get(U64 zkey);
  void Put(int score,
           NodeType node_type,
           int depth,
           U64 zkey,
           Move best_move);
  void Reset();

  void LogStats() const;

 private:
  void Set(int score,
           NodeType node_type,
           int depth,
           U64 zkey,
           Move best_move,
           TranspositionTableEntry* t);

  int hash(const U64 key) const {
    return key % size_;
  }

  int hash2(const U64 key) const {
    return (key >> 1) % size_;
  }

  double UtilizationFactor() const;

  const int size_;

  TranspositionTable2Entry* tentries_;

  unsigned int transpos1_hits_;
  unsigned int transpos2_hits_;
  unsigned int transpos_misses_;
};

}  // namespace search

#endif
