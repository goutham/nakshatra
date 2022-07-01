#ifndef TRANSPOS_H
#define TRANSPOS_H

#include "common.h"
#include "move.h"
#include "zobrist.h"

#include <cstdio>

struct TTEntry {
  U64 zkey;
  Move best_move;
  int16_t score;
  uint8_t depth;
  uint8_t epoch;
  // LSB 0 -> 0: invalid, 1: valid
  // LSB (1, 2) -> 00: FAIL_HIGH_NODE, 01: FAIL_LOW_NODE, 10: EXACT_NODE
  uint16_t flags = 0;

  bool is_valid() const { return flags & 0x1; }

  NodeType node_type() const { return NodeType((flags >> 1) & 0x3); }
};

static_assert(sizeof(TTEntry) == 16);

struct TTBucket {
  TTEntry tt_entries[4];
};

class TranspositionTable {
public:
  TranspositionTable(int size);
  ~TranspositionTable();

  TTEntry Get(U64 zkey, bool* found);

  void Put(int score, NodeType node_type, int depth, U64 zkey, Move best_move);

  void SetEpoch(uint8_t epoch) { epoch_ = epoch; }

  void LogStats() const;

private:
  void Set(int score, NodeType node_type, int depth, U64 zkey, Move best_move,
           TTEntry* t);

  int hash(const U64 key) const { return key % size_; }

  double UtilizationFactor() const;

  const int size_;
  TTBucket* tt_buckets_;
  uint8_t epoch_ = 0;
  uint64_t hits_ = 0;
  uint64_t misses_ = 0;
  uint64_t new_puts_ = 0;
  uint64_t old_replace_ = 0;
  uint64_t depth_replace_ = 0;
};

#endif
