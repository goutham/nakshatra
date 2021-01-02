#ifndef STATS_H
#define STATS_H

#include "common.h"

// Stats for search operations.
struct SearchStats {
  U64 nodes_searched = 0ULL;
  U64 search_depth = 0ULL;
};

#endif
