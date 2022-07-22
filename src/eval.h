#ifndef EVAL_H
#define EVAL_H

#include "board.h"
#include "common.h"

template <Variant variant>
int Evaluate(Board* board, int alpha, int beta);

template <Variant variant>
int EvalResult(Board* board);

#endif
