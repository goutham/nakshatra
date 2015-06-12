#include "board.h"
#include "common.h"
#include "egtb_gen.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

using std::map;
using std::string;
using std::vector;

int main(int argc, char** argv) {
  std::ifstream ifs(argv[1]);
  std::vector<string> egtb;
  string s;
  while (getline(ifs, s)) {
    egtb.push_back(s);
  }

  map<string, EGTBElement> egtb_map;

  for (vector<string>::iterator iter = egtb.begin(); iter != egtb.end();
       ++iter) {
    vector<string> parts = SplitString(*iter, '|');
    EGTBElement elem;
    elem.fen = parts[0];
    elem.moves_to_end = StringToInt(parts[2]);
    Move next_move;
    if (parts[1] != "LOST") {
      next_move = Move(parts[1]);
    }
    elem.next_move = next_move;
    if (egtb_map.find(elem.fen) != egtb_map.end()) {
      std::cerr << "Found same FEN more than once" << std::endl;
      exit(0);
    }
    egtb_map[elem.fen] = elem;
  }

  for (map<string, EGTBElement>::iterator iter = egtb_map.begin();
       iter != egtb_map.end(); ++iter) {
    string fen = iter->first;
    int moves_to_end = iter->second.moves_to_end;
    Move next_move = iter->second.next_move;
    Board board(Variant::SUICIDE, fen);
    while (moves_to_end) {
      board.MakeMove(next_move);
      string newfen = board.ParseIntoFEN();
      map<string, EGTBElement>::iterator egtb_elem = egtb_map.find(newfen);
      if (egtb_elem == egtb_map.end()) {
        std::cerr << "Made move " << next_move.str() << " on board " << fen
             << " but did not find " << newfen << " in the EGTB." << std::endl;
        exit (0);
      }
      if (moves_to_end - 1 != egtb_elem->second.moves_to_end) {
        std::cerr << "Expected " << moves_to_end - 1 << " while found "
             << egtb_elem->second.moves_to_end << std::endl;
        std::cerr << fen << " " << next_move.str() << " " << newfen << std::endl;
        exit(0);
      }
      next_move = egtb_elem->second.next_move;
      --moves_to_end;
    }
  }
  return 0;
}
