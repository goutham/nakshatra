#ifndef EXTENSIONS_H
#define EXTENSIONS_H

class Timer;

namespace search {
class LMR;
class MoveOrderer;
class PNSearch;
}

struct Extensions {
  search::MoveOrderer* move_orderer = nullptr;
  Timer* pns_timer = nullptr;
  search::PNSearch* pn_search = nullptr;
  search::LMR* lmr = nullptr;
};

#endif
