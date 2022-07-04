#include "eval_antichess.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "movegen.h"
#include "piece.h"
#include "stopwatch.h"

#include <cstdlib>

namespace {
constexpr int MOBILITY_FACTOR = 25;
constexpr int PIECE_COUNT_FACTOR = -50;
constexpr int TEMPO = 250;
} // namespace

namespace pv = antichess::piece_value;

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
    auto egtb = GetEGTB(Variant::ANTICHESS);
    if (egtb) {
      const EGTBIndexEntry* egtb_entry = egtb->Lookup(*board_);
      if (egtb_entry) {
        return EGTBResult(*egtb_entry);
      }
    }
    if (RivalBishopsOnOppositeColoredSquares()) {
      return DRAW;
    }
  }

  const int self_moves = CountMoves(Variant::ANTICHESS, board_);
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  if (self_moves == 1) {
    MoveArray move_array;
    GenerateMoves(Variant::ANTICHESS, board_, &move_array);
    board_->MakeMove(move_array.get(0));
    const int eval = -EvaluateInternal(-beta, -alpha, max_depth);
    board_->UnmakeLastMove();
    return eval;
  }
  if (self_moves <= 3 && max_depth > 0) {
    MoveArray move_array;
    GenerateMoves(Variant::ANTICHESS, board_, &move_array);
    int score = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      board_->MakeMove(move_array.get(i));
      const int eval =
          -EvaluateInternal(-beta, -alpha, max_depth - (self_moves - 1));
      board_->UnmakeLastMove();
      if (eval > score) {
        score = eval;
        if (score > alpha) {
          alpha = score;
          if (alpha >= beta) {
            return alpha;
          }
        }
      }
    }
    return score;
  }

  board_->FlipSideToMove();
  const int opp_moves = CountMoves(Variant::ANTICHESS, board_);
  board_->FlipSideToMove();
  if (opp_moves == 0) {
    MoveArray move_array;
    GenerateMoves(Variant::ANTICHESS, board_, &move_array);
    int score = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      const Move& move = move_array.get(i);
      board_->MakeMove(move);
      const int eval = -Evaluate(-beta, -alpha);
      board_->UnmakeLastMove();
      if (eval > score) {
        score = eval;
        if (score > alpha) {
          alpha = score;
          if (alpha >= beta) {
            return alpha;
          }
        }
      }
    }
    return score;
  }

  int score = 0;
  for (Piece p = KING; p <= PAWN; ++p) {
    score += PopCount(board_->BitBoard(p)) * pv::value[p] -
             PopCount(board_->BitBoard(-p)) * pv::value[p];
  }
  if (board_->SideToMove() == Side::BLACK) {
    score = -score;
  }
  return TEMPO + (self_moves - opp_moves) * MOBILITY_FACTOR +
         (self_pieces - opp_pieces) * PIECE_COUNT_FACTOR + score;
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

  const int self_moves = CountMoves(Variant::ANTICHESS, board_);
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  return UNKNOWN;
}
