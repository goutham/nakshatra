#ifndef PN_SEARCH_H
#define PN_SEARCH_H

#include "common.h"
#include "move.h"

#include <iostream>
#include <vector>

#define INF_NODES INT_MAX

class Board;
class EGTB;
class Evaluator;
class MoveGenerator;
class Timer;

typedef int PNSNodeOffset;

struct PNSNode {
  int proof = 1;
  int disproof = 1;
  // Move made by the parent leading to this node, valid for all nodes except
  // root node.
  Move move;
  PNSNode* parent = nullptr;
  std::vector<PNSNode*> children;

  // Number of nodes in the subtree rooted at this node.
  uint64_t tree_size = 1ULL;
};

struct PNSResult {
  struct MoveStat {
    Move move;
    double score;  // lower the better
    uint64_t tree_size;
    int result;
  };
  int result = UNKNOWN;
  int num_nodes = 0;
  std::vector<MoveStat> ordered_moves;
};

struct PNSParams {
  enum PNSearchType {
    PN1,
    PN2
  };
  PNSearchType pns_type = PN1;

  // Maximum number of nodes in PNS tree.
  int max_nodes = 100000;

  // Used only if pns_type = PN2 and pn2_full_search = false. See
  // PnNodes() method implementation for how these are used.
  double pn2_max_nodes_fraction_a = 0.001;
  double pn2_max_nodes_fraction_b = 0.001;

  // Prints progress (in percentage of nodes searched out of max_nodes) after
  // every 'n' secs given by this variable if > 0.
  int log_progress = -1;
};

class PNSearch {
 public:
  // timer_ and egtb may be null.
  // if timer_ is null - PNSearch is not time bound.
  PNSearch(Board* board,
           MoveGenerator* movegen,
           Evaluator* evaluator,
           EGTB* egtb,
           Timer* timer) :
      board_(board),
      movegen_(movegen),
      evaluator_(evaluator),
      egtb_(egtb),
      timer_(timer) {}

  ~PNSearch() {
    if (pns_tree_) {
      Delete(pns_tree_);
      pns_tree_ = nullptr;
    }
  }

  void Search(const PNSParams& pns_params, PNSResult* pns_result);

 private:
  void Expand(const PNSParams& pns_params,
              const int num_nodes,
              const int pns_node_depth,
              PNSNode* pns_node);

  void Pns(const int search_nodes, const PNSParams& pns_params,
           PNSNode* pns_root, int* num_nodes);

  int PnNodes(const PNSParams& pns_params, const int num_nodes);

  bool RedundantMoves(PNSNode* pns_node);

  PNSNode* FindMpn(PNSNode* pns_node, int* depth);

  PNSNode* UpdateAncestors(const PNSParams& pns_params,
                           PNSNode* mpn,
                           PNSNode* pns_root,
                           int* depth);

  void UpdateTreeSize(PNSNode* pns_node);

  void Delete(PNSNode* pns_node);
  void Delete(std::vector<PNSNode*>& pns_nodes);

  Board* board_;
  MoveGenerator* movegen_;
  Evaluator* evaluator_;
  EGTB* egtb_;
  Timer* timer_;

  PNSNode* pns_tree_ = nullptr;
};

#endif
