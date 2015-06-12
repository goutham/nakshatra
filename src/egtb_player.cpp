#include "board.h"
#include "common.h"
#include "egtb_gen.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

int main(int argc, char** argv) {
  std::ifstream ifs(argv[1]);

  map<string, EGTBElement> egtb_map;

  string s;
  while (getline(ifs, s)) {
    vector<string> parts = SplitString(s, '|');
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

  std::cout << "Enter FEN: ";
  getline(std::cin, s);

  map<string, EGTBElement>::iterator egtb_iter = egtb_map.find(s);
  if (egtb_iter == egtb_map.end()) {
    std::cerr << "Not found" << std::endl;
    exit(0);
  }

  while (true) {
    Board board(Variant::SUICIDE, egtb_iter->first);
    board.DebugPrintBoard();
    std::cout << "--> " << egtb_iter->second.moves_to_end << " "
         << egtb_iter->second.next_move.str() << std::endl;
    getchar();
    board.MakeMove(egtb_iter->second.next_move);
    string fen = board.ParseIntoFEN();
    egtb_iter = egtb_map.find(fen);
  }
  return 0;
}
