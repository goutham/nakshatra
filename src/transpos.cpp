#include "transpos.h"
#include "common.h"
#include "stopwatch.h"
#include "zobrist.h"

#include <iostream>
#include <optional>

uint64_t ZKey(const TTEntry& tt_entry) {
  const uint64_t& data = reinterpret_cast<const uint64_t&>(tt_entry.data);
  return tt_entry.zkey ^ data;
}

void SetZKey(int64_t zkey, TTEntry& tt_entry) {
  const uint64_t data = reinterpret_cast<const uint64_t&>(tt_entry.data);
  tt_entry.zkey = (zkey ^ data);
}

TranspositionTable::TranspositionTable(int size) : size_(size) {
  std::cout << "# Transposition table memory usage: "
            << (size_ * sizeof(TTBucket)) / (1U << 20) << " MB" << std::endl;
  tt_buckets_ = new TTBucket[size_];
}

TranspositionTable::~TranspositionTable() { delete tt_buckets_; }

std::optional<TTData> TranspositionTable::Get(U64 zkey) {
  TTBucket& bucket = tt_buckets_[hash(zkey)];
  for (int i = 0; i < 4; ++i) {
    TTEntry tt_entry = bucket.tt_entries[i];
    if (tt_entry.data.is_valid() && ZKey(tt_entry) == zkey) {
      tt_entry.data.epoch = epoch_;
      SetZKey(zkey, tt_entry);
      ++hits_;
      bucket.tt_entries[i] = tt_entry;
      return tt_entry.data;
    }
  }
  ++misses_;
  return std::nullopt;
}

void TranspositionTable::Put(int score, NodeType node_type, int depth, U64 zkey,
                             Move best_move) {
  TTBucket& bucket = tt_buckets_[hash(zkey)];
  for (int i = 0; i < 4; ++i) {
    TTEntry& tt_entry = bucket.tt_entries[i];
    if (!tt_entry.data.is_valid() || ZKey(tt_entry) == zkey) {
      ++new_puts_;
      Set(score, node_type, depth, zkey, best_move, tt_entry);
      return;
    }
  }
  for (int i = 0; i < 4; ++i) {
    TTEntry& tt_entry = bucket.tt_entries[i];
    if (tt_entry.data.epoch < epoch_) {
      ++old_replace_;
      Set(score, node_type, depth, zkey, best_move, tt_entry);
      return;
    }
  }
  TTEntry* shallow_entry = &bucket.tt_entries[0];
  for (int i = 1; i < 4; ++i) {
    TTEntry& tt_entry = bucket.tt_entries[i];
    if (tt_entry.data.depth < shallow_entry->data.depth) {
      shallow_entry = &tt_entry;
    }
  }
  ++depth_replace_;
  Set(score, node_type, depth, zkey, best_move, *shallow_entry);
}

void TranspositionTable::Set(int score, NodeType node_type, int depth, U64 zkey,
                             Move best_move, TTEntry& tt_entry) {
  tt_entry.data.best_move = best_move;
  tt_entry.data.score = score;
  tt_entry.data.depth = depth;
  tt_entry.data.epoch = epoch_;
  tt_entry.data.flags = (uint16_t(node_type) << 1) | (0x1);
  SetZKey(zkey, tt_entry);
}

double TranspositionTable::UtilizationFactor() const {
  unsigned int num_filled_entries = 0;
  for (int i = 0; i < size_; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (tt_buckets_[i].tt_entries[j].data.is_valid()) {
        ++num_filled_entries;
      }
    }
  }
  return (100.0 * num_filled_entries) / (size_ * 4);
}

void TranspositionTable::LogStats() const {
  using std::cout;
  using std::endl;
  cout << "# Transpos hits:\t" << hits_ << endl;
  cout << "# Transpos misses:\t" << misses_ << endl;
  cout << "# Transpos util %:\t" << UtilizationFactor() << endl;
  if (hits_ + misses_ > 0) {
    cout << "# Transpos hits %:\t" << (100.0 * (hits_)) / (hits_ + misses_)
         << endl;
  }
  cout << "# Transpos new entries:\t" << new_puts_ << endl;
  cout << "# Transpos old replace:\t" << old_replace_ << endl;
  cout << "# Transpos depth replace:\t" << depth_replace_ << endl;
}
