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

inline Parameters AsParameters(StdEvalParams<Variable>& fparams) {
  Parameters params;
  for (size_t i = 0; i < fparams.pv_mgame.size(); ++i) {
    params.Add(fparams.pv_mgame[i]);
  }
  for (size_t i = 0; i < fparams.pv_egame.size(); ++i) {
    params.Add(fparams.pv_egame[i]);
  }
  for (size_t i = 0; i < fparams.pst_mgame.size(); ++i) {
    for (size_t j = 0; j < fparams.pst_mgame[i].size(); ++j) {
      params.Add(fparams.pst_mgame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.pst_egame.size(); ++i) {
    for (size_t j = 0; j < fparams.pst_egame[i].size(); ++j) {
      params.Add(fparams.pst_egame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.doubled_pawns_mgame.size(); ++i) {
    params.Add(fparams.doubled_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.doubled_pawns_egame.size(); ++i) {
    params.Add(fparams.doubled_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.passed_pawns_mgame.size(); ++i) {
    params.Add(fparams.passed_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.passed_pawns_egame.size(); ++i) {
    params.Add(fparams.passed_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.isolated_pawns_mgame.size(); ++i) {
    params.Add(fparams.isolated_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.isolated_pawns_egame.size(); ++i) {
    params.Add(fparams.isolated_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.mobility_mgame.size(); ++i) {
    for (size_t j = 0; j < fparams.mobility_mgame[i].size(); ++j) {
      params.Add(fparams.mobility_mgame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.mobility_egame.size(); ++i) {
    for (size_t j = 0; j < fparams.mobility_egame[i].size(); ++j) {
      params.Add(fparams.mobility_egame[i][j]);
    }
  }
  return params;
}
#endif