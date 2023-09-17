#ifndef EGTB_H
#define EGTB_H

#include "board.h"
#include "move.h"

#include <string>
#include <unordered_map>
#include <vector>

struct EGTBIndexEntry {
  uint16_t moves_to_end;
  Move next_move;
  // '1' if the next side to move wins.
  // '-1' if the next side to move loses.
  // '0' if it's a draw.
  int8_t result;
};

class EGTB final {
public:
  void Initialize();

  const EGTBIndexEntry* Lookup(const Board& board);

  void LogStats();

private:
  bool initialized_ = false;
  std::unordered_map<int, std::vector<EGTBIndexEntry>> egtb_index_;
  uint64_t egtb_hits_ = 0ULL;
  uint64_t egtb_misses_ = 0ULL;
};

void PrintEGTBIndexEntry(const EGTBIndexEntry& entry);
int EGTBResult(const EGTBIndexEntry& entry);

int ComputeBoardDescriptionId(const Board& board);
U64 ComputeEGTBIndex(const Board& board);

EGTB* GetEGTB(const Variant variant);

#endif
