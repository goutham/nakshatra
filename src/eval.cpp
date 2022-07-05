#include "eval.h"
#include "board.h"
#include "common.h"

int Evaluate(const Variant variant, Board* board, int alpha, int beta) {
  if (variant == Variant::STANDARD) {
    return Evaluate<Variant::STANDARD>(board, alpha, beta);
  } else {
    assert(variant == Variant::ANTICHESS);
    return Evaluate<Variant::ANTICHESS>(board, alpha, beta);
  }
}

int EvalResult(const Variant variant, Board* board) {
  if (variant == Variant::STANDARD) {
    return EvalResult<Variant::STANDARD>(board);
  } else {
    assert(variant == Variant::ANTICHESS);
    return EvalResult<Variant::ANTICHESS>(board);
  }
}
