#include "board.h"
#include "common.h"
#include "eval.h"
#include "fen.h"
#include "pawns.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Must provide epd file.");
  }
  std::ifstream file(argv[1]);
  std::string str;
  while (std::getline(file, str)) {
    const auto& splits = SplitString(str, ' ');
    const std::string fen =
        splits[0] + " " + splits[1] + " " + splits[2] + " " + splits[3];
    Board board(Variant::STANDARD, fen);
    const int wdpawns = PopCount(pawns::DoubledPawns<Side::WHITE>(board));
    const int bdpawns = PopCount(pawns::DoubledPawns<Side::BLACK>(board));
    const int wppawns = PopCount(pawns::PassedPawns<Side::WHITE>(board));
    const int bppawns = PopCount(pawns::PassedPawns<Side::BLACK>(board));
    std::cout << str << " c0 \""
              << "wdpawns=" << wdpawns << ",bdpawns=" << bdpawns
              << ",wppawns=" << wppawns << ",bppawns=" << bppawns << "\";"
              << std::endl;
  }
  return 0;
}

