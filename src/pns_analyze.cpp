#include "board.h"
#include "common.h"
#include "move.h"
#include "movegen.h"
#include "pn_search.h"
#include "san.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

PNSParams::PNSearchType GetPNSType(const char* arg) {
  if (strcmp(arg, "pn1") == 0) {
    return PNSParams::PN1;
  } else if (strcmp(arg, "pn2") == 0) {
    return PNSParams::PN2;
  } else {
    throw std::invalid_argument("Invalid argument " + std::string(arg));
  }
}

std::string GetPosition(const char* arg) {
  constexpr Variant variant = Variant::ANTICHESS;
  Board board(variant);
  const auto move_str_vec = SplitString(arg, ' ');
  for (const auto& move_str : move_str_vec) {
    const Move move = SANToMove<variant>(move_str, board);
    if (!move.is_valid()) {
      throw std::invalid_argument("Invalid move " + move_str);
    }
    board.MakeMove(move);
  }
  return board.ParseIntoFEN();
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    std::cerr << "Expect arguments: pn1/pn2 <max nodes> <move seq>\n"
              << "Eg: ./pns_analyze pn1 100000 \"e3 b6\"" << std::endl;
    return 0;
  }
  const auto pns_type = GetPNSType(argv[1]);
  const int max_nodes = atoi(argv[2]);
  const std::string position = GetPosition(argv[3]);

  Board board(Variant::ANTICHESS, position);

  PNSParams pns_params;
  pns_params.max_nodes = max_nodes;
  pns_params.pns_type = pns_type;
  pns_params.quiet = false;
  pns_params.log_progress = 10;
  const PNSResult pns_result =
      PNSearch<Variant::ANTICHESS>(board, nullptr, nullptr, nullptr)
          .Search(pns_params);
  std::cout << "tree_size: " << pns_result.pns_tree->tree_size << "\n"
            << "proof: " << pns_result.pns_tree->proof << "\n"
            << "disproof: " << pns_result.pns_tree->disproof << std::endl;

  return 0;
}
