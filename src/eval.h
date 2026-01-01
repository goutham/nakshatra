#ifndef EVAL_H
#define EVAL_H

#include "board.h"
#include "common.h"
#include "egtb.h"
#include "std_eval_params.h"
#include "std_static_eval.h"
#include "params/params.h"

template <Variant variant>
  requires(IsStandard(variant))
int Evaluate(Board& board, EGTB* egtb, int alpha, int beta);

template <Variant variant>
  requires(IsAntichessLike(variant))
int Evaluate(Board& board, EGTB* egtb, int alpha, int beta);

template <Variant variant>
  requires(IsStandard(variant))
int EvalResult(Board& board);

template <Variant variant>
  requires(IsAntichessLike(variant))
int EvalResult(Board& board);

inline int StaticEval(Board& board) {
  static const StdEvalParams<int> params = BlessedParams();
  return standard::StaticEval(params, board);
}

#endif
