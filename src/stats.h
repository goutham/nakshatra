#ifndef STATS_H
#define STATS_H

namespace search {

// Stats for search operations.
struct SearchStats {
  unsigned nodes_searched = 0U;
  unsigned nodes_researched = 0U;
  unsigned nodes_evaluated = 0U;
  unsigned search_depth = 0U;
};

}  // namespace search

#endif
