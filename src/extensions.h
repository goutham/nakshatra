#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <memory>

class PNSearch;
class Timer;

struct PNSExtension {
  std::unique_ptr<PNSearch> pn_search;
  std::unique_ptr<Timer> pns_timer;
  int pns_time_for_move_percent = 5;
};

struct Extensions {
  PNSExtension pns_extension;
};

#endif
