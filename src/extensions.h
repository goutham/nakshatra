#ifndef EXTENSIONS_H
#define EXTENSIONS_H

class LMR;
class MoveOrderer;
class PNSearch;
class Timer;

struct Extensions {
  MoveOrderer* move_orderer = nullptr;
  Timer* pns_timer = nullptr;
  PNSearch* pn_search = nullptr;
  LMR* lmr = nullptr;
};

#endif
