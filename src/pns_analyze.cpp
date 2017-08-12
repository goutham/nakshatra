#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "eval_suicide.h"
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
  Board board(Variant::SUICIDE);
  MoveGeneratorSuicide movegen(board);
  const auto move_str_vec = SplitString(arg, ' ');
  for (const auto& move_str : move_str_vec) {
    const Move move = SANToMove(move_str, board, &movegen);
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

  Board board(Variant::SUICIDE, position);
  MoveGeneratorSuicide movegen(board);

  std::vector<std::string> egtb_filenames;
  assert(GlobFiles("egtb/*.egtb", &egtb_filenames));
  EGTB egtb(egtb_filenames, board);
  egtb.Initialize();
  EvalSuicide eval(&board, &movegen, &egtb);

  PNSParams pns_params;
  pns_params.max_nodes = max_nodes;
  pns_params.pns_type = pns_type;
  pns_params.quiet = false;
  pns_params.log_progress = 10;
  PNSearch pn_search(&board, &movegen, &eval, &egtb, nullptr);
  PNSResult pns_result;
  pn_search.Search(pns_params, &pns_result);
  std::cout << "tree_size: " << pns_result.pns_tree->tree_size << "\n"
            << "proof: " << pns_result.pns_tree->proof << "\n"
            << "disproof: " << pns_result.pns_tree->disproof << std::endl;

  return 0;
}
