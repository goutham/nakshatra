#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <memory>

class LMR;
class MoveOrderer;
class PNSearch;
class Timer;

struct PNSExtension {
  std::unique_ptr<PNSearch> pn_search;
  std::unique_ptr<Timer> pns_timer;
  int pns_time_for_move_percent = 5;
};

struct Extensions {
  std::unique_ptr<MoveOrderer> move_orderer;
  std::unique_ptr<LMR> lmr;
  PNSExtension pns_extension;
};

#endif
