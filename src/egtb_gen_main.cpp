#include "board.h"
#include "common.h"
#include "egtb_gen.h"

#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>

void GeneratePermutations(const std::vector<Piece>& pieces,
                          const std::vector<Side>& next_player_sides,
                          size_t start_index, Board* board,
                          std::set<std::string>* permutations) {
  if (start_index == pieces.size()) {
    for (const Side side : next_player_sides) {
      board->SetPlayerColor(side);
      permutations->insert(board->ParseIntoFEN());
    }
    return;
  }
  Piece piece = pieces.at(start_index);
  for (int i = 0; i < 8; i++) {
    if (PieceType(piece) == PAWN && (i == 0 || i == 7)) {
      continue;
    }
    for (int j = 0; j < 8; j++) {
      if (board->PieceAt(i, j) == NULLPIECE) {
        board->SetPiece(INDX(i, j), piece);
        GeneratePermutations(pieces, next_player_sides, start_index + 1, board,
                             permutations);
        board->SetPiece(INDX(i, j), NULLPIECE);
      }
    }
  }
}

void All1p(std::set<std::string>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    GeneratePermutations({piece}, {Side::BLACK}, 0, &board, positions);
    GeneratePermutations({-piece}, {Side::WHITE}, 0, &board, positions);
  }
}

void All2p(std::set<std::string>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    for (Piece piece2 = KING; piece2 <= PAWN; ++piece2) {
      GeneratePermutations({piece, piece2}, {Side::BLACK}, 0, &board,
                           positions);
      GeneratePermutations({-piece, -piece2}, {Side::WHITE}, 0, &board,
                           positions);
      GeneratePermutations({piece, -piece2}, {Side::BLACK, Side::WHITE}, 0,
                           &board, positions);
    }
  }
}

void All3p(std::set<std::string>* positions) {
  Board board(Variant::ANTICHESS, "8/8/8/8/8/8/8/8 w - -");
  for (Piece piece = KING; piece <= PAWN; ++piece) {
    for (Piece piece2 = KING; piece2 <= PAWN; ++piece2) {
      for (Piece piece3 = KING; piece3 <= PAWN; ++piece3) {
        GeneratePermutations({piece, piece2, piece3}, {Side::BLACK}, 0, &board,
                             positions);
        GeneratePermutations({-piece, -piece2, -piece3}, {Side::WHITE}, 0,
                             &board, positions);
        GeneratePermutations({piece, piece2, -piece3},
                             {Side::BLACK, Side::WHITE}, 0, &board, positions);
        GeneratePermutations({-piece, -piece2, piece3},
                             {Side::BLACK, Side::WHITE}, 0, &board, positions);
      }
    }
  }
}

int main() {
  std::set<std::string> positions;
  All1p(&positions);
  All2p(&positions);
  // All3p(&positions);
  std::cout << "Number of positions: " << positions.size() << std::endl;
  auto positions_list =
      std::list<std::string>(positions.begin(), positions.end());
  positions.clear();
  std::cout << "Generating EGTB..." << std::endl;
  EGTBStore store;
  EGTBGenerate(positions_list, &store);
  std::cout << "Writing out..." << std::endl;
  store.Write();
  std::cout << "Done." << std::endl;
  return 0;
}
