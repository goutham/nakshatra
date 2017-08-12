#ifndef MOVE_ARRAY_H
#define MOVE_ARRAY_H

#include "move.h"

class MoveArray {
public:
  MoveArray() : n_(0u) {}

  void Add(const Move& move) { moves_[n_++] = move; }

  void SwapToFront(const Move& move) {
    size_t index_to_swap = 0;
    for (size_t i = 0; i < n_; ++i) {
      if (moves_[i] == move) {
        index_to_swap = i;
        break;
      }
    }
    if (index_to_swap != 0) {
      moves_[index_to_swap] = moves_[0];
      moves_[0] = move;
    }
  }

  void PushToFront(const Move& move) {
    size_t index = 0;
    for (size_t i = 0; i < n_; ++i) {
      if (moves_[i] == move) {
        index = i;
        break;
      }
    }
    while (index) {
      moves_[index] = moves_[index - 1];
      --index;
    }
    moves_[index] = move;
  }

  bool Contains(const Move& move) const {
    for (size_t i = 0; i < n_; ++i) {
      if (moves_[i] == move)
        return true;
    }
    return false;
  }

  size_t size() const { return n_; }

  const Move& get(size_t index) const { return moves_[index]; }

  void clear() { n_ = 0; }

private:
  Move moves_[256];
  size_t n_;
};

#endif
