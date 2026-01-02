#ifndef STD_EVAL_PARAMS_H
#define STD_EVAL_PARAMS_H

#include <array>
#include <optional>

#include "tuning/mlp.h"
#include "tuning/variable.h"

template <typename ValueType>
using PieceValues = std::array<ValueType, 7>;

template <typename ValueType>
using PST = std::array<std::array<ValueType, 64>, 7>;

template <typename ValueType>
struct MLPValueType {
  using T = ValueType;
};

template <>
struct MLPValueType<int> {
  using T = double;
};

template <typename ValueType>
struct StdEvalParams {
  PieceValues<ValueType> pv_mgame = {};
  PieceValues<ValueType> pv_egame = {};
  PST<ValueType> pst_mgame = {};
  PST<ValueType> pst_egame = {};
  std::array<ValueType, 64> doubled_pawns_mgame = {};
  std::array<ValueType, 64> doubled_pawns_egame = {};
  std::array<ValueType, 64> passed_pawns_mgame = {};
  std::array<ValueType, 64> passed_pawns_egame = {};
  std::array<ValueType, 64> isolated_pawns_mgame = {};
  std::array<ValueType, 64> isolated_pawns_egame = {};
  std::array<ValueType, 64> defended_pawns_mgame = {};
  std::array<ValueType, 64> defended_pawns_egame = {};
  PST<ValueType> mobility_mgame = {};
  PST<ValueType> mobility_egame = {};
  ValueType tempo_w_mgame = 0;
  ValueType tempo_w_egame = 0;
  ValueType tempo_b_mgame = 0;
  ValueType tempo_b_egame = 0;
  std::optional<MLP<typename MLPValueType<ValueType>::T>> pawn_struct_mlp = MLP<typename MLPValueType<ValueType>::T>(128, {32, 8, 1}, {});
  ValueType pawn_struct_w_mgame = 0;
  ValueType pawn_struct_w_egame = 0;
  ValueType pawn_struct_b_mgame = 0;
  ValueType pawn_struct_b_egame = 0;
};

#endif
