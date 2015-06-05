#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval_suicide.h"
#include "movegen.h"
#include "movegen_suicide.h"
#include "piece.h"
#include "stopwatch.h"

#include <cstdlib>

namespace {
// Piece values.
namespace pv {
const int KING = 10;
const int QUEEN = 3;
const int ROOK = 7;
const int BISHOP = 2;
const int KNIGHT = 3;
const int PAWN = 3;
}  // namespace pv

// Weight for mobility.
const int MOBILITY_FACTOR = 25;
}

int EvalSuicide::PieceValDifference() const {
  const int white_val =
      PopCount(board_->BitBoard(KING))    * pv::KING   +
      PopCount(board_->BitBoard(QUEEN))   * pv::QUEEN  +
      PopCount(board_->BitBoard(PAWN))    * pv::PAWN   +
      PopCount(board_->BitBoard(BISHOP))  * pv::BISHOP +
      PopCount(board_->BitBoard(KNIGHT))  * pv::KNIGHT +
      PopCount(board_->BitBoard(ROOK))    * pv::ROOK;
  const int black_val =
      PopCount(board_->BitBoard(-KING))   * pv::KING   +
      PopCount(board_->BitBoard(-QUEEN))  * pv::QUEEN  +
      PopCount(board_->BitBoard(-PAWN))   * pv::PAWN   +
      PopCount(board_->BitBoard(-BISHOP)) * pv::BISHOP +
      PopCount(board_->BitBoard(-KNIGHT)) * pv::KNIGHT +
      PopCount(board_->BitBoard(-ROOK))   * pv::ROOK;

  return (board_->SideToMove() == Side::WHITE) ?
         (white_val - black_val) :
         (black_val - white_val);
}

// Returns the mobility of opponent. This is calculated as the least possible
// number of moves that opponent can be forced to make by current playing side.
// Should only be called when self mobility is non-zero.
// Special case: If no current side move leaves the opponent with any move to
// play, return INF.
int EvalSuicide::OpponentMobility(const MoveArray& move_array) {
  // This function should not be called when self mobility is 0.
  assert(move_array.size() > 0);
  int best = INF;
  for (unsigned i = 0; i < move_array.size(); ++i) {
    board_->MakeMove(move_array.get(i));
    unsigned num_opp_moves = CountMoves(board_->SideToMove(), *board_);
    board_->UnmakeLastMove();
    if (num_opp_moves == 1) {  // Best case scenario; return immediately.
      return num_opp_moves;
    }
    if (num_opp_moves > 0 && num_opp_moves < best) {
      best = num_opp_moves;
    }
  }
  return best;
}

// Inspect result of a move where opponent_mobility = INF.
int EvalSuicide::InspectResultOfMove() {
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  assert(move_array.size() != 0);
  int result = -INF;
  for (unsigned i = 0; i < move_array.size(); ++i) {
    board_->MakeMove(move_array.get(i));
    // Inverse of opponent's perspective.
    int opponent_pieces = board_->NumPieces(board_->SideToMove());
    int self_pieces = board_->NumPieces(OppositeSide(board_->SideToMove()));
    board_->UnmakeLastMove();
    if (opponent_pieces == self_pieces) {
      if (result < DRAW) result = DRAW;
    } else if (opponent_pieces < self_pieces) {
      if (result < -WIN) result = -WIN;
    } else {
      result = WIN;
      break;
    }
  }
  return result;
}

bool EvalSuicide::RivalBishopsOnOppositeColoredSquares() const {
  static const U64 WHITE_SQUARES = 0xAA55AA55AA55AA55ULL;
  static const U64 BLACK_SQUARES = 0x55AA55AA55AA55AAULL;

  const U64 white_bishop = board_->BitBoard(BISHOP);
  const U64 black_bishop = board_->BitBoard(-BISHOP);

  return (white_bishop && black_bishop &&
      ((white_bishop & WHITE_SQUARES) && (black_bishop & BLACK_SQUARES)) ||
      ((white_bishop & BLACK_SQUARES) && (black_bishop & WHITE_SQUARES)));
}

int EvalSuicide::Evaluate() {
  int self_pieces = board_->NumPieces(board_->SideToMove());
  int opponent_pieces = board_->NumPieces(OppositeSide(board_->SideToMove()));

  if (self_pieces == 1 && opponent_pieces == 1) {
    if (egtb_) {
      const EGTBIndexEntry* egtb_entry = egtb_->Lookup();
      if (egtb_entry) {
        return EGTBResult(*egtb_entry);
      }
    }
    if (RivalBishopsOnOppositeColoredSquares()) {
      return DRAW;
    }
  }

  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  int self_mobility = move_array.size();

  // If there are no pieces to move, it is the end of the game.
  if (self_mobility == 0) {
    if (self_pieces < opponent_pieces) return WIN;
    else if (self_pieces == opponent_pieces) return DRAW;
    return -WIN;
  }

  if (self_mobility == 1) {
    board_->MakeMove(move_array.get(0));
    int eval_val = -Evaluate();
    board_->UnmakeLastMove();
    return eval_val;
  }

  int opponent_mobility = OpponentMobility(move_array);

  // Current board position could be bad for current side as no self move allows
  // opponent to make a move. We need to evaluate the result of this move to
  // verify whether it is actually bad or good.
  if (opponent_mobility == INF) {
    return InspectResultOfMove();
  }

  return PieceValDifference() +
         MOBILITY_FACTOR * (self_mobility - opponent_mobility);
}

int EvalSuicide::Result() const {
  int self_pieces = board_->NumPieces(board_->SideToMove());
  int opponent_pieces = board_->NumPieces(OppositeSide(board_->SideToMove()));

  if (self_pieces == 1 && opponent_pieces == 1 &&
      RivalBishopsOnOppositeColoredSquares()) {
    return DRAW;
  }

  int self_mobility = CountMoves(board_->SideToMove(), *board_);

  // If there are no pieces to move, it is the end of the game.
  if (self_mobility == 0) {
    if (self_pieces < opponent_pieces) return WIN;
    else if (self_pieces == opponent_pieces) return DRAW;
    else return -WIN;
  }

  // Non-{WIN, -WIN, DRAW} value suffices.
  return UNKNOWN;
}
