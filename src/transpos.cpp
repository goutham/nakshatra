#include "common.h"
#include "stopwatch.h"
#include "transpos.h"
#include "zobrist.h"

#include <iostream>

namespace search {

TranspositionTable::TranspositionTable(int size) :
    size_(size),
    transpos1_hits_(0U),
    transpos2_hits_(0U),
    transpos_misses_(0U) {
  std::cout << "# Transposition table memory usage: "
            << (size_ * sizeof(TranspositionTable2Entry)) / (1U << 20) << " MB"
            << std::endl;
  tentries_ = new TranspositionTable2Entry[size_];
}

TranspositionTable::~TranspositionTable() {
  Reset();
}

TranspositionTableEntry* TranspositionTable::Get(U64 zkey) {
  TranspositionTable2Entry* tentry = &tentries_[hash(zkey)];
  if (tentry->t1.valid && tentry->t1.zkey == zkey) {
    ++transpos1_hits_;
    return &tentry->t1;
  }
  if (tentry->t2.valid && tentry->t2.zkey == zkey) {
    ++transpos2_hits_;
    return &tentry->t2;
  }
  ++transpos_misses_;
  return nullptr;
}

void TranspositionTable::Put(int score,
                             NodeType node_type,
                             int depth,
                             U64 zkey,
                             Move best_move) {
  TranspositionTable2Entry* tentry = &tentries_[hash(zkey)];
  TranspositionTableEntry* t = nullptr;
  if (!tentry->t1.valid || tentry->t1.depth < depth) {
    t = &tentry->t1;
  } else {
    t = &tentry->t2;
  }
  Set(score, node_type, depth, zkey, best_move, t);
}

void TranspositionTable::Set(int score, NodeType node_type, int depth, U64 zkey,
                             Move best_move, TranspositionTableEntry* t) {
  t->valid = true;
  t->score = score;
  t->node_type = node_type;
  t->depth = depth;
  t->zkey = zkey;
  t->best_move = best_move;
}

void TranspositionTable::Reset() {
  delete tentries_;
}

double TranspositionTable::UtilizationFactor() const {
  unsigned int num_filled_entries = 0;
  for (int i = 0; i < size_; ++i) {
    if (tentries_[i].t1.valid || tentries_[i].t2.valid) {
      ++num_filled_entries;
    }
  }
  return (100.0 * num_filled_entries) / size_;
}

void TranspositionTable::LogStats() const {
  using std::cout;
  using std::endl;
  cout << "# Transpos 1 hits:\t" << transpos1_hits_ << endl;
  cout << "# Transpos 2 hits:\t" << transpos2_hits_ << endl;
  cout << "# Transpos misses:\t" << transpos_misses_ << endl;
  cout << "# Transpos utilization factor:\t" << UtilizationFactor() << " %"
       << endl;
  if (transpos1_hits_ + transpos2_hits_ + transpos_misses_ > 0) {
    cout << "# Percentage of hits:\t"
         << (100.0 * (transpos1_hits_ + transpos2_hits_)) /
         (transpos1_hits_ + transpos2_hits_ + transpos_misses_) << " %"
         << endl;
  }
}

}  // namespace search
