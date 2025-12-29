#ifndef __PARAMS_H__
#define __PARAMS_H__

#include "common.h"
#include "params/MobilityParamsEpoch13Step8500.h"
#include "params/OrigParams.h"
#include "params/Params20241117Epoch99Step63720.h"
#include "params/ExpTempo202502Epoch4Step2000.h"
#include "params/ZeroParams.h"
#include "std_eval_params.h"

inline StdEvalParams<int> BlessedParams() {
  return ExpTempo202502Epoch4Step2000Int();
}

inline StdEvalParams<double> BlessedParamsDbl() {
  return ExpTempo202502Epoch4Step2000Dbl();
}

#endif
