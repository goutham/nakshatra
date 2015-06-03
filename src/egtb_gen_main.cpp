#include "board.h"
#include "common.h"
#include "egtb_gen.h"
#include "movegen.h"
#include "piece.h"
#include "move.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define MAX 10000

using std::string;
using std::vector;

void GeneratePermutations(const vector<string>& initial_list,
                          Piece piece, vector<string>* permutations) {
  for (const string& fen : initial_list) {
    Board board(SUICIDE, fen);
    for (int i = 0; i < 8; ++i) {
      if (PieceType(piece) == PAWN && (i == 0 || i == 7)) {
        continue;
      }
      for (int j = 0; j < 8; ++j) {
        if (board.PieceAt(i, j) == NULLPIECE) {
          board.SetPiece(INDX(i, j), piece);
          permutations->push_back(board.ParseIntoFEN());
          board.SetPiece(INDX(i, j), NULLPIECE);
        }
      }
    }
  }
}

string AddPlayerSide(const string& fen, Side side) {
  Board board(SUICIDE, fen);
  board.SetPlayerColor(side);
  return board.ParseIntoFEN();
}

void CreateTwoPiecesEGTB(Side winning_side,
                         Side losing_side,
                         EGTBStore* store) {
  vector<string> empty_board_list(
      {Board(SUICIDE, "8/8/8/8/8/8/8/8 w - -").ParseIntoFEN()});

  vector<string> one_list;
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    vector<string> tmp_one_list;
    GeneratePermutations(empty_board_list,
                         PieceOfSide(piece, losing_side),
                         &tmp_one_list);
    one_list.insert(one_list.end(),
                    tmp_one_list.begin(),
                    tmp_one_list.end());
  }
  vector<string> tmp_list;
  for (const string& fen : one_list) {
    tmp_list.push_back(AddPlayerSide(fen, winning_side));
  }
  one_list.swap(tmp_list);

  vector<string> two_list;
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    vector<string> tmp_two_list;
    GeneratePermutations(one_list,
                         PieceOfSide(piece, winning_side),
                         &tmp_two_list);
    two_list.insert(two_list.end(),
                    tmp_two_list.begin(),
                    tmp_two_list.end());
  }
  tmp_list.clear();
  for (const string& fen : two_list) {
    tmp_list.push_back(AddPlayerSide(fen, losing_side));
    tmp_list.push_back(AddPlayerSide(fen, winning_side));
  }
  two_list.swap(tmp_list);
  std::cout << two_list.size() << std::endl;

  EGTBGenerator egtb_gen;
  egtb_gen.Generate(one_list, two_list, winning_side, store);
}


int main() {
  EGTBStore store, store2;
  CreateTwoPiecesEGTB(Side::BLACK, Side::WHITE, &store);
  CreateTwoPiecesEGTB(Side::WHITE, Side::BLACK, &store2);
  store.MergeFrom(store2);
  std::ofstream ofs("2p.text.egtb", std::ofstream::out);
  store.Write(ofs);

  return 0;
}
