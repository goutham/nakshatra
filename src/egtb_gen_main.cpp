#include "board.h"
#include "common.h"
#include "egtb_gen.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <vector>

#define MAX 10000

using std::list;
using std::string;
using std::vector;

void GeneratePermutations(const string& fen, const Piece piece,
                          const vector<Side> next_player_sides,
                          list<string>* permutations) {
  Board board(Variant::SUICIDE, fen);
  for (int i = 0; i < 8; ++i) {
    if (PieceType(piece) == PAWN && (i == 0 || i == 7)) {
      continue;
    }
    for (int j = 0; j < 8; ++j) {
      if (board.PieceAt(i, j) == NULLPIECE) {
        board.SetPiece(INDX(i, j), piece);
        for (const Side next_player_side : next_player_sides) {
          board.SetPlayerColor(next_player_side);
          permutations->push_back(board.ParseIntoFEN());
        }
        board.SetPiece(INDX(i, j), NULLPIECE);
      }
    }
  }
}

void GeneratePermutations(const list<string>& initial_list, const Piece piece,
                          const vector<Side> next_player_sides,
                          list<string>* permutations) {
  for (const string& fen : initial_list) {
    GeneratePermutations(fen, piece, next_player_sides, permutations);
  }
}

void CreateTwoPiecesEGTB(Side winning_side, Side losing_side,
                         EGTBStore* store) {
  Board empty_board(Variant::SUICIDE, "8/8/8/8/8/8/8/8 w - -");

  list<string> one_piece_positions;
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    GeneratePermutations(empty_board.ParseIntoFEN(),
                         PieceOfSide(piece, losing_side), {winning_side},
                         &one_piece_positions);
  }

  list<string> two_piece_positions;
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    GeneratePermutations(one_piece_positions, PieceOfSide(piece, winning_side),
                         {Side::WHITE, Side::BLACK}, &two_piece_positions);
  }

  list<string> all_positions;
  all_positions.insert(all_positions.end(),
                       std::make_move_iterator(one_piece_positions.begin()),
                       std::make_move_iterator(one_piece_positions.end()));
  all_positions.insert(all_positions.end(),
                       std::make_move_iterator(two_piece_positions.begin()),
                       std::make_move_iterator(two_piece_positions.end()));

  EGTBGenerate(all_positions, store);
}

int main() {
  EGTBStore store, store2;
  CreateTwoPiecesEGTB(Side::BLACK, Side::WHITE, &store);
  CreateTwoPiecesEGTB(Side::WHITE, Side::BLACK, &store2);
  store.MergeFrom(store2);
  store.Write();

  return 0;
}
