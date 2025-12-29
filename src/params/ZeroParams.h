#ifndef ZeroParams_h
#define ZeroParams_h

#include "std_eval_params.h"
#include <array>

inline StdEvalParams<int> ZeroParams() {
  static const StdEvalParams<int> int_params{
      .pv_mgame = {0, 20000, 900, 500, 300, 300, 100},
      .pv_egame = {0, 20000, 900, 500, 300, 300, 100},
  };
  return int_params;
}

#endif
