#include "see.h"
#include "attacks.h"
#include "board.h"
#include "common.h"
#include "move.h"
#include "piece.h"

// #include <iostream>

// using namespace std;

// void log(int depth, int gain, Piece captured_piece, Piece capturing_piece) {
//   cout << "At depth " << depth << " " << PieceToChar(capturing_piece)
//        << " captures " << PieceToChar(captured_piece) << " for gain of " <<
//        gain
//        << endl;
// }

// TODO: These are for testing only, not actually used by eval. Merge them.
namespace pv {
constexpr int KING = 20000;
constexpr int QUEEN = 900;
constexpr int ROOK = 500;
constexpr int BISHOP = 300;
constexpr int KNIGHT = 300;
constexpr int PAWN = 100;
constexpr int value[] = {0, KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
} // namespace pv

bool LeastValuableAttacker(const int sq, const Side side, const U64 occ,
                           const Board& board, U64* attacker, Piece* piece) {
  for (Piece p = PAWN; p >= KING; --p) {
    Piece ps = PieceOfSide(p, side);
    U64 attackers =
        attacks::SquareAttackers(sq, ps, occ, board.BitBoard(ps) & occ);
    *attacker = attackers & -attackers;
    if (*attacker) {
      *piece = ps;
      return true;
    }
  }
  return false;
}

int SEE(const Move move, const Board& board) {
  const int from_sq = move.from_index();
  const int to_sq = move.to_index();
  Piece capturing_piece = board.PieceAt(from_sq);
  Piece captured_piece = board.PieceAt(to_sq);
  int gain[32];
  int depth = 0;
  gain[depth] = pv::value[PieceType(captured_piece)];
  // log(depth, gain[depth], captured_piece, capturing_piece);
  U64 attacker = (1ULL << from_sq);
  U64 occ = board.BitBoard() ^ attacker;
  Side side = OppositeSide(board.SideToMove());
  Piece piece = NULLPIECE;
  while (LeastValuableAttacker(to_sq, side, occ, board, &attacker, &piece)) {
    occ ^= attacker;
    captured_piece = capturing_piece;
    capturing_piece = piece;
    side = OppositeSide(side);
    ++depth;
    gain[depth] = pv::value[PieceType(captured_piece)] - gain[depth - 1];
    // log(depth, gain[depth], captured_piece, capturing_piece);
  }
  while (depth--) {
    gain[depth] = -std::max(-gain[depth], gain[depth + 1]);
  }
  return gain[0];
}