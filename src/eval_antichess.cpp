#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "movegen.h"
#include "piece_values.h"
#include "stopwatch.h"

#include <cstdlib>

namespace {

constexpr int MOBILITY_FACTOR = 25;
constexpr int PIECE_COUNT_FACTOR = -50;
constexpr int TEMPO = 250;
constexpr int EVAL_MAX_DEPTH = 5;

bool RivalBishopsOnOppositeColoredSquares(Board* board) {
  static const U64 WHITE_SQUARES = 0xAA55AA55AA55AA55ULL;
  static const U64 BLACK_SQUARES = 0x55AA55AA55AA55AAULL;

  const U64 white_bishop = board->BitBoard(BISHOP);
  const U64 black_bishop = board->BitBoard(-BISHOP);

  return ((white_bishop && black_bishop) &&
          (((white_bishop & WHITE_SQUARES) && (black_bishop & BLACK_SQUARES)) ||
           ((white_bishop & BLACK_SQUARES) && (black_bishop & WHITE_SQUARES))));
}

int EvaluateInternal(Board* board, int alpha, int beta,
                     int max_depth = EVAL_MAX_DEPTH) {
  const Side side = board->SideToMove();
  const int self_pieces = board->NumPieces(side);
  const int opp_pieces = board->NumPieces(OppositeSide(side));

  if (self_pieces == 1 && opp_pieces == 1) {
    auto egtb = GetEGTB(Variant::ANTICHESS);
    if (egtb) {
      const EGTBIndexEntry* egtb_entry = egtb->Lookup(*board);
      if (egtb_entry) {
        return EGTBResult(*egtb_entry);
      }
    }
    if (RivalBishopsOnOppositeColoredSquares(board)) {
      return DRAW;
    }
  }

  const int self_moves = CountMoves<Variant::ANTICHESS>(board);
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  if (self_moves == 1) {
    MoveArray move_array;
    GenerateMoves<Variant::ANTICHESS>(board, &move_array);
    board->MakeMove(move_array.get(0));
    const int eval = -EvaluateInternal(board, -beta, -alpha, max_depth);
    board->UnmakeLastMove();
    return eval;
  }
  if (self_moves <= 3 && max_depth > 0) {
    MoveArray move_array;
    GenerateMoves<Variant::ANTICHESS>(board, &move_array);
    int score = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      board->MakeMove(move_array.get(i));
      const int eval =
          -EvaluateInternal(board, -beta, -alpha, max_depth - (self_moves - 1));
      board->UnmakeLastMove();
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

  board->FlipSideToMove();
  const int opp_moves = CountMoves<Variant::ANTICHESS>(board);
  board->FlipSideToMove();
  if (opp_moves == 0) {
    MoveArray move_array;
    GenerateMoves<Variant::ANTICHESS>(board, &move_array);
    int score = -INF;
    for (size_t i = 0; i < move_array.size(); ++i) {
      const Move& move = move_array.get(i);
      board->MakeMove(move);
      const int eval = -EvaluateInternal(board, -beta, -alpha);
      board->UnmakeLastMove();
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
    score += PopCount(board->BitBoard(p)) * antichess::PIECE_VALUES[p] -
             PopCount(board->BitBoard(-p)) * antichess::PIECE_VALUES[p];
  }
  if (board->SideToMove() == Side::BLACK) {
    score = -score;
  }
  return TEMPO + (self_moves - opp_moves) * MOBILITY_FACTOR +
         (self_pieces - opp_pieces) * PIECE_COUNT_FACTOR + score;
}

} // namespace

template <>
int Evaluate<Variant::ANTICHESS>(Board* board, int alpha, int beta) {
  return EvaluateInternal(board, alpha, beta);
}

template <>
int EvalResult<Variant::ANTICHESS>(Board* board) {
  const Side side = board->SideToMove();
  const int self_pieces = board->NumPieces(side);
  const int opp_pieces = board->NumPieces(OppositeSide(side));

  if (self_pieces == 1 && opp_pieces == 1 &&
      RivalBishopsOnOppositeColoredSquares(board)) {
    return DRAW;
  }

  const int self_moves = CountMoves<Variant::ANTICHESS>(board);
  if (self_moves == 0) {
    return self_pieces < opp_pieces ? WIN
                                    : (self_pieces == opp_pieces ? DRAW : -WIN);
  }

  return UNKNOWN;
}
