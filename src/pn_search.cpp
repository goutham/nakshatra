#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "movegen.h"
#include "pn_search.h"
#include "stopwatch.h"
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <unistd.h>
#include <utility>
#include <vector>

#define PNS_MAX_DEPTH 600

void PNSearch::Search(const PNSParams& pns_params, PNSResult* pns_result) {
  if (pns_tree_) {
    Delete(pns_tree_);
    pns_tree_ = nullptr;
  }
  pns_tree_ = new PNSNode;

  Pns(pns_params, pns_tree_);
  pns_result->pns_tree = pns_tree_;
  pns_result->tree_size = pns_tree_->tree_size;

  for (PNSNode* pns_node : pns_tree_->children) {
    // This is from the current playing side perspective.
    double score;
    int result;
    if (pns_node->proof == 0) {
      score = DBL_MAX;
      result = -WIN;
    } else {
      score = static_cast<double>(pns_node->disproof) / pns_node->proof;
      if (pns_node->proof == INF_NODES && pns_node->disproof == 0) {
        result = WIN;
      } else if (pns_node->proof == INF_NODES &&
                 pns_node->disproof == INF_NODES) {
        result = DRAW;
      } else {
        result = UNKNOWN;
      }
    }
    pns_result->ordered_moves.push_back({
        pns_node->move,
        score,
        pns_node->tree_size,
        result});
  }
  sort(pns_result->ordered_moves.begin(), pns_result->ordered_moves.end(),
       [] (const PNSResult::MoveStat& a, const PNSResult::MoveStat& b) {
         return a.score < b.score;
       });
  // Print the ordered moves.
  if (!pns_params.quiet) {
    std::cout << "# Move, score, tree_size:" << std::endl;
    for (const auto& move_stat : pns_result->ordered_moves) {
      static std::map<int, std::string> result_map = {
          {WIN, "WIN"}, {-WIN, "LOSS"}, {DRAW, "DRAW"}, {UNKNOWN, "UNKNOWN"}};
      std::cout << "# " << move_stat.move.str() << ", " << move_stat.score
                << ", " << move_stat.tree_size << ", "
                << result_map.at(move_stat.result) << std::endl;
    }
  }
}

void PNSearch::Pns(const PNSParams& pns_params, PNSNode* pns_root) {
  PNSNode* cur_node = pns_root;

  StopWatch stop_watch;
  stop_watch.Start();

  int depth = 0, num_nodes = 0;
  int log_progress_secs = pns_params.log_progress;
  while (num_nodes < pns_params.max_nodes &&
         (pns_root->proof != 0 && pns_root->disproof != 0) &&
         (!timer_ || !timer_->timer_expired())) {
    if (pns_params.log_progress > 0 &&
        stop_watch.ElapsedTime() / 100 > log_progress_secs) {
      std::cout << "# Progress: "
           << (100.0 * num_nodes) / pns_params.max_nodes
           << "% ("
           << num_nodes << " / " << pns_params.max_nodes
           << ")" << std::endl;
      log_progress_secs += pns_params.log_progress;
    }
    PNSNode* mpn = FindMpn(cur_node, &depth);
    Expand(pns_params, num_nodes, depth, mpn);
    num_nodes += mpn->children.size();
    cur_node = UpdateAncestors(pns_params, mpn, pns_root, &depth);
  }
  while (cur_node != pns_root) {
    cur_node = cur_node->parent;
    --depth;
    assert (board_->UnmakeLastMove());
    UpdateTreeSize(cur_node);
  }
  assert(depth == 0);
}

bool PNSearch::RedundantMoves(PNSNode* pns_node) {
  if (pns_node &&
      pns_node->parent &&
      pns_node->parent->parent &&
      pns_node->parent->parent->parent) {
    const Move& m1 = pns_node->move;
    const Move& m2 = pns_node->parent->move;
    const Move& m3 = pns_node->parent->parent->move;
    const Move& m4 = pns_node->parent->parent->parent->move;
    if (m1.from_index() == m3.to_index() &&
        m1.to_index() == m3.from_index() &&
        m2.from_index() == m4.to_index() &&
        m2.to_index() == m4.from_index()) {
      return true;
    }
  }
  return false;
}

PNSNode* PNSearch::FindMpn(PNSNode* root, int* depth) {
  PNSNode* mpn = root;
  while (!mpn->children.empty()) {
    // If proof number of parent node is INF_NODES, all it's children will have
    // disproof number of INF_NODES. So, select the child node that has a proof
    // number that is not 0 (i.e, not yet proved). Otherwise, we may end up
    // reaching a leaf node that is proved/disproved/drawn with no scope for
    // expansion.
    if (mpn->proof == INF_NODES) {
      for (PNSNode* pns_node : mpn->children) {
        if (pns_node->proof) {
          mpn = pns_node;
          break;
        }
      }
    } else {
      for (PNSNode* pns_node : mpn->children) {
        if (mpn->proof == pns_node->disproof) {
          mpn = pns_node;
          break;
        }
      }
    }
    ++*depth;
    board_->MakeMove(mpn->move);
  }
  assert(mpn->children.empty());
  return mpn;
}

PNSNode* PNSearch::UpdateAncestors(const PNSParams& pns_params,
                                   PNSNode* mpn,
                                   PNSNode* pns_root,
                                   int* depth) {
  PNSNode* pns_node = mpn;
  while (true) {
    if (!pns_node->children.empty()) {
      int proof = INF_NODES;
      int disproof = 0;
      pns_node->tree_size = 1;
      for (PNSNode* child : pns_node->children) {
        if (child->disproof < proof) {
          proof = child->disproof;
        }
        if (child->proof == INF_NODES) {
          disproof = INF_NODES;
        } else if (disproof != INF_NODES) {
          disproof += child->proof;
        }
        pns_node->tree_size += child->tree_size;
      }
      // Terminate updating ancestors if proof/disproof numbers
      // don't change and it is not MPN in a PN^2 higher level
      // tree. In PN^2, MPN will have unevaluated children due to
      // use of delayed evaluation so we must continue even if
      // proof/disproof don't change.
      if (pns_node->proof == proof &&
          pns_node->disproof == disproof &&
          (pns_params.pns_type != PNSParams::PN2 ||
           pns_node != mpn)) {
        return pns_node;
      }
      pns_node->proof = proof;
      pns_node->disproof = disproof;
    }
    if (pns_node == pns_root) {
      return pns_node;
    }
    pns_node = pns_node->parent;
    --*depth;
    assert(board_->UnmakeLastMove());
  }
  assert(false);
}

void PNSearch::UpdateTreeSize(PNSNode* pns_node) {
  if (!pns_node->children.empty()) {
    pns_node->tree_size = 1;
    for (PNSNode* child : pns_node->children) {
      pns_node->tree_size += child->tree_size;
    }
  }
}

void PNSearch::Expand(const PNSParams& pns_params,
                      const int num_nodes,
                      const int pns_node_depth,
                      PNSNode* pns_node) {
  if (RedundantMoves(pns_node) || pns_node_depth >= PNS_MAX_DEPTH) {
    pns_node->proof = INF_NODES;
    pns_node->disproof = INF_NODES;
    assert(pns_node->children.empty());
  } else if (pns_params.pns_type == PNSParams::PN2) {
    PNSParams pns_params2;
    pns_params2.pns_type = PNSParams::PN1;
    pns_params2.max_nodes = PnNodes(pns_params, num_nodes);
    Pns(pns_params2, pns_node);

    // If the tree is solved, delete the entire Pn subtree under
    // the pns_node. Else, retain MPN's immediate children only.
    if (pns_node->proof == 0 || pns_node->disproof == 0) {
      Delete(pns_node->children);
      pns_node->tree_size = 1;
    } else {
      for (PNSNode* child : pns_node->children) {
        Delete(child->children);
        child->tree_size = 1;
      }
      pns_node->tree_size = 1 + pns_node->children.size();
    }
  } else {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    for (size_t i = 0; i < move_array.size(); ++i) {
      PNSNode* child = new PNSNode;
      pns_node->children.push_back(child);
      child->move = move_array.get(i);
      child->parent = pns_node;
      board_->MakeMove(child->move);
      int result = evaluator_->Result();
      if (result == UNKNOWN &&
          egtb_ &&
          OnlyOneBitSet(board_->BitBoard(Side::WHITE)) &&
          OnlyOneBitSet(board_->BitBoard(Side::BLACK))) {
        const EGTBIndexEntry* egtb_entry = egtb_->Lookup();
        if (egtb_entry) {
          result = EGTBResult(*egtb_entry);
        }
      }
      if (result == DRAW) {
        child->proof = INF_NODES;
        child->disproof = INF_NODES;
      } else if (result == -WIN) {
        child->proof = INF_NODES;
        child->disproof = 0;
      } else if (result == WIN) {
        child->proof = 0;
        child->disproof = INF_NODES;
      } else {
        child->proof = 1;
        child->disproof = movegen_->CountMoves();
      }
      board_->UnmakeLastMove();
    }
    pns_node->tree_size = 1 + pns_node->children.size();
  }
}

int PNSearch::PnNodes(const PNSParams& pns_params,
                      const int num_nodes) {
  const double a = pns_params.pn2_max_nodes_fraction_a * pns_params.max_nodes;
  const double b = pns_params.pn2_max_nodes_fraction_b * pns_params.max_nodes;
  const double f_x = 1.0 / (1.0 + exp((a - num_nodes) / b));
  return static_cast<int>(
      std::min(ceil(std::max(num_nodes, 1) * f_x),
                   static_cast<double>(pns_params.max_nodes - num_nodes)));
}

void PNSearch::Delete(PNSNode* pns_node) {
  Delete(pns_node->children);
  delete pns_node;
}

void PNSearch::Delete(std::vector<PNSNode*>& pns_nodes) {
  for (PNSNode* pns_node : pns_nodes) {
    Delete(pns_node);
  }
  pns_nodes.clear();
}
