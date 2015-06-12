#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "eval_suicide.h"
#include "lmr.h"
#include "move.h"
#include "movegen.h"
#include "pn_search.h"
#include "stats.h"

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, const char** argv) {
  srand(time(NULL));

  Board board(Variant::SUICIDE);
  MoveGeneratorSuicide movegen(board);
  EGTB egtb("2p.bin.egtb", board);
  egtb.Initialize();
  EvalSuicide eval(&board, &movegen, &egtb);
  PNSParams pns_params;
  pns_params.pns_type = PNSParams::PN2;
  pns_params.pn2_full_search = true;
  pns_params.save_progress = 50000;
  pns_params.log_progress = 10;
  PNSearch pn_search(200000,
                     &board,
                     &movegen,
                     &eval,
                     &egtb,
                     nullptr);
  PNSResult pns_result;
  pn_search.Search(pns_params, &pns_result);
  return 0;
}
