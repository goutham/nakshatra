#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"
#include "move.h"
#include <algorithm>
#include <cstring>

class HistoryTable {
public:
  HistoryTable() { Clear(); }

  void Clear() {
    std::memset(table_, 0, sizeof(table_));
  }

  void Decay() {
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 64; ++j) {
        for (int k = 0; k < 64; ++k) {
          table_[i][j][k] /= 2;
        }
      }
    }
  }

  int Get(Side side, int from, int to) const {
    return table_[SideIndex(side)][from][to];
  }

  void Set(Side side, int from, int to, int value) {
    table_[SideIndex(side)][from][to] = value;
  }

  void Update(Side side, const Move& move, int depth, bool is_bonus) {
    const int MAX_HISTORY = 16384;
    const int bonus = std::min(depth * depth, 1200);
    const int from = move.from_index();
    const int to = move.to_index();
    int& val = table_[SideIndex(side)][from][to];

    if (is_bonus) {
      val += bonus - (val * bonus / MAX_HISTORY);
    } else {
      val -= bonus + (val * bonus / MAX_HISTORY);
    }
  }

private:
  int table_[2][64][64];
};

#endif
