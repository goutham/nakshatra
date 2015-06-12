#include "board.h"
#include "common.h"
#include "egtb.h"
#include "egtb_gen.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  std::ifstream ifs("2p.text.egtb");
  std::ofstream ofs("2p.bin.egtb", std::ofstream::binary);

  std::string s;
  while (getline(ifs, s)) {
    EGTBIndexEntry egtb_index_entry;
    std::vector<std::string> parts = SplitString(s, '|');
    std::string fen = parts[0];
    Board board(SUICIDE, fen);
    egtb_index_entry.next_move = Move(parts[1]);
    egtb_index_entry.moves_to_end = StringToInt(parts[2]);
    if ((board.SideToMove() == Side::BLACK &&
         parts[3][0] == 'B') ||
        (board.SideToMove() == Side::WHITE &&
         parts[3][0] == 'W')) {
      egtb_index_entry.result = 1;
    } else if ((board.SideToMove() == Side::BLACK &&
                parts[3][0] == 'W') ||
               (board.SideToMove() == Side::WHITE &&
                parts[3][0] == 'B')) {
      egtb_index_entry.result = -1;
    } else {
      // No other case supported.
      assert(false);
    }
    if (OnlyOneBitSet(board.BitBoard(Side::WHITE)) &&
        OnlyOneBitSet(board.BitBoard(Side::BLACK))) {
      int64_t index = GetIndex_1_1(board);
      ofs.seekp(index * sizeof(EGTBIndexEntry),
                std::ios_base::beg);
      ofs.write(reinterpret_cast<char*>(&egtb_index_entry),
                sizeof(EGTBIndexEntry));
    }
  }
  ifs.close();
  ofs.close();
  return 0;
}
