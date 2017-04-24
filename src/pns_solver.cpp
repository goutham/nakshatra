#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "eval_suicide.h"
#include "lmr.h"
#include "move.h"
#include "movegen.h"
#include "pn_search.h"
#include "san.cpp"
#include "stats.h"

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Node {
  Move move;
  double score;
  double probability;
  std::vector<Node*> children;
};

double Score(const PNSNode* node) {
  if (node->disproof == 0) {
    return DBL_MAX;
  }
  return static_cast<double>(node->proof) / node->disproof;
}

template <typename T>
void DeleteTree(T* tree) {
  if (!tree) {
    return;
  }
  if (tree->children.empty()) {
    delete tree;
    return;
  }
  for (auto* child : tree->children) {
    DeleteTree(child);
  }
  delete tree;
}

Node* ConstructOpeningsTree(const PNSNode* pns_tree, int depth) {
  if (!pns_tree || depth <= 0 || pns_tree->proof == 0) {
    return nullptr;
  }

  Node* root = new Node;
  root->move = pns_tree->move;
  root->score = Score(pns_tree);

  double total = 0.0;
  for (auto* pns_child : pns_tree->children) {
    Node* child = ConstructOpeningsTree(pns_child, depth - 1);
    if (child) {
      root->children.push_back(child);
      total += child->score;
    }
  }

  // Calculate probabilities and sort (high to low).
  for (auto* child : root->children) {
    child->probability = child->score / total;
  }
  sort(root->children.begin(), root->children.end(),
       [](const Node* a, const Node* b) {
         return a->probability > b->probability;
       });

  // Eliminate children with probability lower than kProbThreshold.
  double kProbThreshold = 0.05;
  std::vector<Node*> new_children;
  for(auto* child : root->children) {
    if (child->probability > kProbThreshold) {
      new_children.push_back(child);
    } else {
      DeleteTree(child);
    }
  }
  root->children.swap(new_children);

  // Recalculate probabilities. Sorted order is retained.
  total = 0.0;
  for (auto* child : root->children) {
    total += child->score;
  }
  for (auto* child : root->children) {
    child->probability = child->score / total;
  }

  return root;
}

void PrintSpaces(std::ofstream& ofs, int n) {
  for (int i = 0; i < n; ++i) {
    ofs << " ";
  }
}

void WriteTree(std::ofstream& ofs, Board* board, Node* tree,
               int spaces) {
  if (!tree) {
    return;
  }
  for (Node* child : tree->children) {
    ofs << "\n";
    PrintSpaces(ofs, spaces);
    ofs << "(";
    const string move_san = SAN(*board, child->move);
    ofs << move_san << std::setprecision(2)
        << " [" << child->probability << ", " << child->score << "]";
    board->MakeMove(child->move);
    WriteTree(ofs, board, child, spaces + 2);
    board->UnmakeLastMove();
    ofs << ")";
  }
}

int main(int argc, const char** argv) {
  Board board(Variant::SUICIDE);
  MoveGeneratorSuicide movegen(board);

  std::vector<std::string> egtb_filenames;
  assert(GlobFiles("egtb/*.egtb", &egtb_filenames));
  EGTB egtb(egtb_filenames, board);
  egtb.Initialize();
  EvalSuicide eval(&board, &movegen, &egtb);

  PNSParams pns_params;
  pns_params.max_nodes = 200000;
  pns_params.pns_type = PNSParams::PN2;
  pns_params.quiet = true;
  pns_params.log_progress = 10;
  PNSearch pn_search(&board,
                     &movegen,
                     &eval,
                     &egtb,
                     nullptr);
  PNSResult pns_result;

  std::cout << "Running PNS..." << std::endl;
  pn_search.Search(pns_params, &pns_result);

  const string kOpeningsFile = "openings.txt";
  std::ofstream ofs(kOpeningsFile.c_str(), std::ios::out);
  PNSNode* pns_tree = pns_result.pns_tree;

  std::cout << "Constructing openings tree..." << std::endl;
  Node* tree = ConstructOpeningsTree(pns_tree, 12);

  std::cout << "Writing to " << kOpeningsFile << "..." << std::endl;
  WriteTree(ofs, &board, tree, 0);

  std::cout << "Cleaning up..." << std::endl;
  ofs.close();
  DeleteTree(tree);

  return 0;
}
