#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cmath>
#include <vector>

#include "tuning/variable.h"

class Parameters {
public:
  void Add(Variable v) { params_.push_back(v); }

  void ApplyGrad(double learning_rate) {
    for (auto& param : params_) {
      param.data_->value -= param.data_->grad * learning_rate;
    }
  }

  void ZeroGrad() {
    for (auto& param : params_) {
      param.data_->grad = 0.0;
    }
  }

  void ApplyMomentum() {
    static constexpr double beta = 0.9;
    for (auto& param : params_) {
      param.data_->velocity =
          param.data_->velocity * beta + (1 - beta) * param.data_->grad;
      param.data_->grad = param.data_->velocity;
    }
  }

  double GradNorm() {
    double sum = 0.0;
    for (auto& param : params_) {
      sum += param.data_->grad * param.data_->grad;
    }
    return std::sqrt(sum);
  }

  double WeightsMag() {
    double sum = 0.0;
    for (auto& param : params_) {
      sum += param.data_->value * param.data_->value;
    }
    return std::sqrt(sum);
  }

  size_t Size() const { return params_.size(); }

private:
  std::vector<Variable> params_;
};

template <size_t N>
void Add(const std::array<Variable, N>& array, Parameters& params) {
  for (size_t i = 0; i < array.size(); ++i) {
    params.Add(array[i]);
  }
}

template <size_t M, size_t N>
void Add(const std::array<std::array<Variable, M>, N>& array, Parameters& params) {
  for (size_t i = 0; i < array.size(); ++i) {
    for (size_t j = 0; j < array[i].size(); ++j) {
      params.Add(array[i][j]);
    }
  }
}

void Add(const Variable& v, Parameters& params) {
  params.Add(v);
}

inline Parameters AsParameters(StdEvalParams<Variable>& fparams) {
  Parameters params;
  Add(fparams.pv_mgame, params);
  Add(fparams.pv_egame, params);
  Add(fparams.pst_mgame, params);
  Add(fparams.pst_egame, params);
  Add(fparams.doubled_pawns_mgame, params);
  Add(fparams.doubled_pawns_egame, params);
  Add(fparams.passed_pawns_mgame, params);
  Add(fparams.passed_pawns_egame, params);
  Add(fparams.isolated_pawns_mgame, params);
  Add(fparams.isolated_pawns_egame, params);
  Add(fparams.mobility_mgame, params);
  Add(fparams.mobility_egame, params);
  Add(fparams.tempo_w_mgame, params);
  Add(fparams.tempo_w_egame, params);
  Add(fparams.tempo_b_mgame, params);
  Add(fparams.tempo_b_egame, params);
  return params;
}
#endif