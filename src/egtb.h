#ifndef EGTB_H
#define EGTB_H

#include "board.h"
#include "move.h"

#include <string>

struct EGTBIndexEntry {
  uint16_t moves_to_end;
  Move next_move;
  // '1' if the next side to move wins.
  // '-1' if the next side to move loses.
  // '0' if it's a draw.
  int8_t result;
};

class EGTB {
 public:
  EGTB(const std::string& egtb_file,
       const Board& board);
  virtual ~EGTB();
  void Initialize();
  int64_t GetIndex();

  const EGTBIndexEntry* Lookup();

  void LogStats();

 private:
  const std::string egtb_file_;
  const Board& board_;
  bool initialized_;
  EGTBIndexEntry* egtb_index_;
  int64_t num_entries_;
  uint64_t egtb_hits_;
  uint64_t egtb_misses_;
};

int64_t GetIndex_1_1(const Board& board);
void PrintEGTBIndexEntry(const EGTBIndexEntry& entry);
int EGTBResult(const EGTBIndexEntry& entry);

int ComputeBoardDescriptionId(const Board& board);
U64 ComputeEGTBIndex(const Board& board);

#endif
