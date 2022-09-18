#ifndef EVAL_H
#define EVAL_H

#include "board.h"
#include "common.h"

template <Variant variant>
  requires(IsStandard(variant))
int Evaluate(Board* board, int alpha, int beta);

template <Variant variant>
  requires(IsAntichessLike(variant))
int Evaluate(Board* board, int alpha, int beta);

template <Variant variant>
  requires(IsStandard(variant))
int EvalResult(Board* board);

template <Variant variant>
  requires(IsAntichessLike(variant))
int EvalResult(Board* board);

#endif
