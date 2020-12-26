#include "eval_antichess.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "movegen.h"
#include "piece.h"
#include "stopwatch.h"

#include <cstdlib>

namespace {
// Piece values.
namespace pv {
constexpr int KING = 10;
constexpr int QUEEN = 6;
constexpr int ROOK = 7;
constexpr int BISHOP = 3;
constexpr int KNIGHT = 3;
constexpr int PAWN = 2;
} // namespace pv

// Weight for mobility.
constexpr int MOBILITY_FACTOR = 25;

constexpr int PIECE_COUNT_FACTOR = -50;

constexpr int TEMPO = 250;
} // namespace

int EvalAntichess::PieceValDifference() const {
  const int white_val = PopCount(board_->BitBoard(KING)) * pv::KING +
                        PopCount(board_->BitBoard(QUEEN)) * pv::QUEEN +
                        PopCount(board_->BitBoard(PAWN)) * pv::PAWN +
                        PopCount(board_->BitBoard(BISHOP)) * pv::BISHOP +
                        PopCount(board_->BitBoard(KNIGHT)) * pv::KNIGHT +
                        PopCount(board_->BitBoard(ROOK)) * pv::ROOK;
  const int black_val = PopCount(board_->BitBoard(-KING)) * pv::KING +
                        PopCount(board_->BitBoard(-QUEEN)) * pv::QUEEN +
                        PopCount(board_->BitBoard(-PAWN)) * pv::PAWN +
                        PopCount(board_->BitBoard(-BISHOP)) * pv::BISHOP +
                        PopCount(board_->BitBoard(-KNIGHT)) * pv::KNIGHT +
                        PopCount(board_->BitBoard(-ROOK)) * pv::ROOK;

  return (board_->SideToMove() == Side::WHITE) ? (white_val - black_val)
                                               : (black_val - white_val);
}

int EvalAntichess::PieceCountDiff() const {
  const int white_count = PopCount(board_->BitBoard(Side::WHITE));
  const int black_count = PopCount(board_->BitBoard(Side::BLACK));
  return (board_->SideToMove() == Side::WHITE) ? (white_count - black_count)
                                               : (black_count - white_count);
}

bool EvalAntichess::RivalBishopsOnOppositeColoredSquares() const {
  static const U64 WHITE_SQUARES = 0xAA55AA55AA55AA55ULL;
  static const U64 BLACK_SQUARES = 0x55AA55AA55AA55AAULL;

  const U64 white_bishop = board_->BitBoard(BISHOP);
  const U64 black_bishop = board_->BitBoard(-BISHOP);

  return ((white_bishop && black_bishop) &&
          (((white_bishop & WHITE_SQUARES) && (black_bishop & BLACK_SQUARES)) ||
           ((white_bishop & BLACK_SQUARES) && (black_bishop & WHITE_SQUARES))));
}

int EvalAntichess::EvaluateInternal(int alpha, int beta, int max_depth) {
  const Side side = board_->SideToMove();
  const int self_pieces = board_->NumPieces(side);
  const int opp_pieces = board_->NumPieces(OppositeSide(side));

  if (self_pieces == 1 && opp_pieces == 1) {
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

  const int self_moves = movegen_->CountMoves();
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  if (self_moves == 1) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    board_->MakeMove(move_array.get(0));
    const int eval_val = -EvaluateInternal(-beta, -alpha, max_depth);
    board_->UnmakeLastMove();
    return eval_val;
  }
  if (self_moves <= 3 && max_depth > 0) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    int score = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      board_->MakeMove(move_array.get(i));
      const int eval_val = -EvaluateInternal(-beta, -alpha, max_depth - (self_moves - 1));
      board_->UnmakeLastMove();
      if (eval_val > score) {
        score = eval_val;
      }
    }
    return score;
  }

  board_->FlipSideToMove();
  const int opp_moves = movegen_->CountMoves();
  board_->FlipSideToMove();
  if (opp_moves == 0) {
    MoveArray move_array;
    movegen_->GenerateMoves(&move_array);
    int max_eval = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      const Move& move = move_array.get(i);
      board_->MakeMove(move);
      const int eval_val = -Evaluate(-beta, -alpha);
      board_->UnmakeLastMove();
      if (max_eval < eval_val) {
        max_eval = eval_val;
      }
    }
    return max_eval;
  }

  return (self_moves - opp_moves) * MOBILITY_FACTOR + PieceValDifference() +
         TEMPO + PIECE_COUNT_FACTOR * PieceCountDiff();
}

int EvalAntichess::Evaluate(int alpha, int beta) {
  return EvaluateInternal(alpha, beta, 5);
}

int EvalAntichess::Result() const {
  const Side side = board_->SideToMove();
  const int self_pieces = board_->NumPieces(side);
  const int opp_pieces = board_->NumPieces(OppositeSide(side));

  if (self_pieces == 1 && opp_pieces == 1 &&
      RivalBishopsOnOppositeColoredSquares()) {
    return DRAW;
  }

  const int self_moves = movegen_->CountMoves();
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  return UNKNOWN;
}
