#include "board.h"
#include "common.h"
#include "params/params.h"
#include "std_static_eval.h"
#include "tuning/layer.h"
#include "tuning/mlp.h"
#include "tuning/neuron.h"
#include "tuning/parameters.h"
#include "tuning/variable.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

const std::string kExperimentName = "PawnStructExp2";
const std::string kDataFile = "/home/goutham/workspace/github/goutham/nakshatra-tools/tuning/main.epd";
constexpr double kMultiplier = 1.0 / 113.6;
constexpr double kLearningRate = 10.0;
constexpr int kBatchSize = 1024;
constexpr int kMaxEpochs = 100;

struct EPDRecord {
  std::string fen;
  double result;

  friend std::ostream& operator<<(std::ostream& os, const EPDRecord& val) {
    os << val.fen << " | " << val.result;
    return os;
  }
};

template <typename FromType, typename ToType>
ToType Cast(FromType from) {
  if constexpr (std::is_same_v<FromType, double> &&
                std::is_same_v<ToType, int>) {
    return static_cast<ToType>(std::round(from));
  }
  return ToType(from);
}

template <typename FromType, typename ToType, size_t N>
void Convert(const std::array<FromType, N>& from, std::array<ToType, N>& to) {
  for (size_t i = 0; i < from.size(); ++i) {
    to[i] = Cast<FromType, ToType>(from[i]);
  }
}

template <typename FromType, typename ToType, size_t M, size_t N>
void Convert(const std::array<std::array<FromType, M>, N>& from,
             std::array<std::array<ToType, M>, N>& to) {
  for (size_t i = 0; i < from.size(); ++i) {
    for (size_t j = 0; j < from[i].size(); ++j) {
      to[i][j] = Cast<FromType, ToType>(from[i][j]);
    }
  }
}

template <typename FromType, typename ToType>
void Convert(const FromType& from, ToType& to) {
  to = Cast<FromType, ToType>(from);
}

template <typename FromType, typename ToType>
void Convert(const std::vector<FromType>& from, std::vector<ToType>& to) {
  to.resize(from.size());
  for (size_t i = 0; i < from.size(); ++i) {
    to[i] = Cast<FromType, ToType>(from[i]);
  }
}

void ConvertMLP(const std::optional<MLP<double>>& from, std::optional<MLP<Variable>>& to) {
  if (!from) {
    to = std::nullopt;
    return;
  }
  to.emplace(from->nin_, from->nouts_, from->Parameters());
}

void ConvertMLP(const std::optional<MLP<Variable>>& from, std::optional<MLP<double>>& to) {
  if (!from) {
    to = std::nullopt;
    return;
  }
  std::vector<double> weights;
  Convert(from->Parameters(), weights);
  to.emplace(from->nin_, from->nouts_, weights);
}

template <typename T>
void ConvertMLP(const std::optional<MLP<T>>& from, std::optional<MLP<T>>& to) {
  if (!from) {
    to = std::nullopt;
    return;
  }
  std::vector<double> weights;
  Convert(from->Parameters(), weights);
  to.emplace(from->nin_, from->nouts_, weights);
}

template <typename FromType, typename ToType>
StdEvalParams<ToType> Convert(const StdEvalParams<FromType>& fparams) {
  StdEvalParams<ToType> params;
  Convert(fparams.pv_mgame, params.pv_mgame);
  Convert(fparams.pv_egame, params.pv_egame);
  Convert(fparams.pst_mgame, params.pst_mgame);
  Convert(fparams.pst_egame, params.pst_egame);
  Convert(fparams.doubled_pawns_mgame, params.doubled_pawns_mgame);
  Convert(fparams.doubled_pawns_egame, params.doubled_pawns_egame);
  Convert(fparams.passed_pawns_mgame, params.passed_pawns_mgame);
  Convert(fparams.passed_pawns_egame, params.passed_pawns_egame);
  Convert(fparams.isolated_pawns_mgame, params.isolated_pawns_mgame);
  Convert(fparams.isolated_pawns_egame, params.isolated_pawns_egame);
  Convert(fparams.defended_pawns_mgame, params.defended_pawns_mgame);
  Convert(fparams.defended_pawns_egame, params.defended_pawns_egame);
  Convert(fparams.mobility_mgame, params.mobility_mgame);
  Convert(fparams.mobility_egame, params.mobility_egame);
  Convert(fparams.tempo_w_mgame, params.tempo_w_mgame);
  Convert(fparams.tempo_w_egame, params.tempo_w_egame);
  Convert(fparams.tempo_b_mgame, params.tempo_b_mgame);
  Convert(fparams.tempo_b_egame, params.tempo_b_egame);
  ConvertMLP(fparams.pawn_struct_mlp, params.pawn_struct_mlp);
  Convert(fparams.pawn_struct_w_mgame, params.pawn_struct_w_mgame);
  Convert(fparams.pawn_struct_w_egame, params.pawn_struct_w_egame);
  Convert(fparams.pawn_struct_b_mgame, params.pawn_struct_b_mgame);
  Convert(fparams.pawn_struct_b_egame, params.pawn_struct_b_egame);
  return params;
}

template <typename ValueType>
std::string GetParamType() {
  if constexpr (std::is_same_v<ValueType, int>) {
    return "int";
  }
  if constexpr (std::is_same_v<ValueType, double>) {
    return "double";
  }
  throw std::invalid_argument("unsupported param type");
}

template <typename ValueType>
std::string GetFnSuffix() {
  if constexpr (std::is_same_v<ValueType, int>) {
    return "Int";
  }
  if constexpr (std::is_same_v<ValueType, double>) {
    return "Dbl";
  }
  throw std::invalid_argument("unsupported param type");
}

template <typename ValueType>
void WriteParams(std::ofstream& ofs, const std::string& name,
                 const PieceValues<ValueType>& pv) {
  const std::string param_type = GetParamType<ValueType>();
  ofs << "  ." << name << " = {";
  for (size_t i = 0; i < pv.size(); ++i) {
    ofs << pv[i] << ", ";
  }
  ofs << "}," << std::endl;
}

template <typename ValueType>
void WriteParams(std::ofstream& ofs, const std::string& name,
                 const PST<ValueType>& pst) {
  const std::string param_type = GetParamType<ValueType>();
  ofs << "  ." << name << " = {";
  for (size_t i = 0; i < pst.size(); ++i) {
    ofs << "std::array<" << param_type << ", 64>{";
    for (size_t j = 0; j < pst[i].size(); ++j) {
      ofs << pst[i][j] << ", ";
    }
    ofs << "}," << std::endl;
  }
  ofs << "}," << std::endl;
}

template <typename ValueType>
void WriteParams(std::ofstream& ofs, const std::string& name,
                 const std::array<ValueType, 64>& array) {
  ofs << "  ." << name << " = {";
  for (size_t i = 0; i < array.size(); ++i) {
    ofs << array[i] << ", ";
  }
  ofs << "}," << std::endl;
}

template <typename ValueType>
void WriteParams(std::ofstream& ofs, const std::string& name,
                 const ValueType& v) {
  ofs << "  ." << name << " = " << v << "," << std::endl;
}

template <typename ValueType>
void WriteParams(std::ofstream& ofs, const std::string& name,
                 const std::optional<MLP<ValueType>>& mlp) {
  if (mlp.has_value()) {
    ofs << "  ." << name << " = MLP<double>(" << mlp->nin_ << ", {";
    for (size_t i = 0; i < mlp->nouts_.size(); ++i) {
      ofs << mlp->nouts_[i] << ", ";
    }
    ofs << "}, {";
    auto params = mlp->Parameters();
    for (size_t i = 0; i < params.size(); ++i) {
      ofs << params[i] << ", ";
    }
    ofs << "})," << std::endl;
  } else {
    ofs << "  ." << name << " = std::nullopt," << std::endl;
  }
}

template <typename ValueType>
void WriteFunction(const StdEvalParams<ValueType>& params,
                   const std::string exp_name, std::ofstream& ofs) {
  const std::string param_type = GetParamType<ValueType>();
  const std::string fn_suffix = GetFnSuffix<ValueType>();
  const std::string fn_name = exp_name + fn_suffix;
  ofs << "inline StdEvalParams<" << param_type << ">" << fn_name << "() {"
      << std::endl;
  ofs << "static const StdEvalParams<" << param_type << "> params{"
      << std::endl;

  WriteParams(ofs, "pv_mgame", params.pv_mgame);
  WriteParams(ofs, "pv_egame", params.pv_egame);
  WriteParams(ofs, "pst_mgame", params.pst_mgame);
  WriteParams(ofs, "pst_egame", params.pst_egame);
  WriteParams(ofs, "doubled_pawns_mgame", params.doubled_pawns_mgame);
  WriteParams(ofs, "doubled_pawns_egame", params.doubled_pawns_egame);
  WriteParams(ofs, "passed_pawns_mgame", params.passed_pawns_mgame);
  WriteParams(ofs, "passed_pawns_egame", params.passed_pawns_egame);
  WriteParams(ofs, "isolated_pawns_mgame", params.isolated_pawns_mgame);
  WriteParams(ofs, "isolated_pawns_egame", params.isolated_pawns_egame);
  WriteParams(ofs, "defended_pawns_mgame", params.defended_pawns_mgame);
  WriteParams(ofs, "defended_pawns_egame", params.defended_pawns_egame);
  WriteParams(ofs, "mobility_mgame", params.mobility_mgame);
  WriteParams(ofs, "mobility_egame", params.mobility_egame);
  WriteParams(ofs, "tempo_w_mgame", params.tempo_w_mgame);
  WriteParams(ofs, "tempo_w_egame", params.tempo_w_egame);
  WriteParams(ofs, "tempo_b_mgame", params.tempo_b_mgame);
  WriteParams(ofs, "tempo_b_egame", params.tempo_b_egame);
  WriteParams(ofs, "pawn_struct_mlp", params.pawn_struct_mlp);
  WriteParams(ofs, "pawn_struct_w_mgame", params.pawn_struct_w_mgame);
  WriteParams(ofs, "pawn_struct_w_egame", params.pawn_struct_w_egame);
  WriteParams(ofs, "pawn_struct_b_mgame", params.pawn_struct_b_mgame);
  WriteParams(ofs, "pawn_struct_b_egame", params.pawn_struct_b_egame);

  ofs << "};" << std::endl;
  ofs << "return params;" << std::endl;
  ofs << "}" << std::endl;
  ofs << std::endl;
}

void WriteFile(const StdEvalParams<double>& params, int epoch, int step,
               const std::string& suffix = "") {
  std::stringstream ss;
  ss << kExperimentName << "Epoch" << epoch << "Step" << step;
  const std::string exp_name = ss.str() + suffix;
  const std::string filename = exp_name + ".h";
  const std::string header_guard = exp_name + "_H";
  std::cout << "Writing parameters to file: " << filename << std::endl;
  std::ofstream ofs(filename);
  ofs << "#ifndef " << header_guard << std::endl;
  ofs << "#define " << header_guard << std::endl;
  ofs << std::endl;
  ofs << "#include \"std_eval_params.h\"" << std::endl;
  ofs << "#include <array>" << std::endl;
  ofs << std::endl;
  WriteFunction(params, exp_name, ofs);
  WriteFunction(Convert<double, int>(params), exp_name, ofs);
  ofs << std::endl;
  ofs << "#endif" << std::endl;
}

std::vector<EPDRecord> Parse() {
  std::ifstream ifs(kDataFile);
  if (!ifs) {
    std::cerr << "Unable to open data file: " << kDataFile << std::endl;
    exit(-1);
  }
  std::vector<EPDRecord> epd_records;
  while (true) {
    std::string fen;
    std::string nextp;
    std::string castling;
    std::string ep;
    std::string comment;
    std::string result;
    if (ifs >> fen) {
      ifs >> nextp;
      ifs >> castling;
      ifs >> ep;
      ifs >> comment;
      assert(comment == "c9");
      ifs >> result;
    } else {
      break;
    }
    EPDRecord epd_record;
    epd_record.fen = fen + " " + nextp + " " + castling + " " + ep;
    if (result == "\"1-0\";") {
      epd_record.result = 1.0;
    } else if (result == "\"0-1\";") {
      epd_record.result = 0.0;
    } else if (result == "\"1/2-1/2\";") {
      epd_record.result = 0.5;
    } else {
      throw std::invalid_argument("unexpected str: " + result);
    }
    // if (nextp == "b") {
    //   epd_record.result = 1.0 - epd_record.result;
    // }
    epd_records.push_back(epd_record);
  }
  return epd_records;
}

template <typename ValueType>
double Loss(double score, double result, const StdEvalParams<ValueType>& params,
            double multiplier = kMultiplier) {
  return pow(result - (1.0 / (1.0 + exp(-score * multiplier))), 2.0);
}

Variable Loss(Variable score, double result,
              const StdEvalParams<Variable>& params,
              double multiplier = kMultiplier) {
  return (Variable(result) - score.Sigmoid(multiplier)).Power(2.0);
}

Variable PawnStructLoss(Variable val, double result) {
  return (Variable(result) - val.Sigmoid()).Power(2.0);
}

double AvgPawnStructLoss(const std::vector<EPDRecord>& epd_records, MLP<Variable>& mlp) {
  double full_loss = 0.0;
  for (const auto& record : epd_records) {
    Board board(Variant::STANDARD, record.fen);
    std::vector<U64> bbs = {board.BitBoard(PAWN), board.BitBoard(-PAWN)};
    double val = double(mlp(bbs)[0]);
    auto loss = (record.result - (1.0 / (1.0 + exp(-val))));
    loss *= loss;
    full_loss += loss;
  }
  return full_loss / epd_records.size();
}

template <typename ValueType>
double AvgLoss(const std::vector<EPDRecord>& epd_records,
               const StdEvalParams<ValueType>& params,
               double multiplier = kMultiplier) {
  double loss = 0.0;
  for (const auto& record : epd_records) {
    Board board(Variant::STANDARD, record.fen);
<<<<<<< HEAD
    const double score = standard::StaticEval<ValueType, false, false>(params, board);
=======
    const double score =
        standard::StaticEval<ValueType, false, false>(params, board);
>>>>>>> master
    loss += double(Loss(score, record.result, params, multiplier));
  }
  return loss / epd_records.size();
}

void LogMetric(int epoch, int step, const std::string& metric, double value) {
  std::cout << "epoch:" << epoch << ", step:" << step << ", metric:" << metric
            << ", value:" << value << std::endl;
}

// best multiplier = 1 / 113.6
void main_find_best_multiplier() {
  StdEvalParams<double> eval_params = Convert<int, double>(OrigParams());
  auto epd_records = Parse();
  double best_loss = 100000.;
  double best_d = -1.0;
  for (double d = 112.0; d <= 115.0; d += 0.1) {
    double loss = 0.0;
    for (auto& record : epd_records) {
      Board board(Variant::STANDARD, record.fen);
      auto score = standard::StaticEval<double, false, false>(eval_params, board);
      loss += Loss(score, record.result, eval_params, 1.0 / d);
    }
    loss = loss / epd_records.size();
    if (loss < best_loss) {
      best_loss = loss;
      best_d = d;
      std::cout << "best_d:" << best_d << ", best_loss:" << best_loss << std::endl;
    } else {
      std::cout << "loss:" << loss << ", d:" << d << std::endl;
    }
  }
}

<<<<<<< HEAD
void main_piece_occupancy_stats() {
  const Piece piece = PAWN;
  auto epd_records = Parse();
  std::array<int, 64> white_piece_stats{};
  std::array<int, 64> black_piece_stats{};
  for (auto& record : epd_records) {
    Board board(Variant::STANDARD, record.fen);
    auto white_piece = board.BitBoard(piece);
    while (white_piece) {
      int index = Lsb1(white_piece);
      white_piece_stats[index]++;
      white_piece ^= (1ULL << index);
    }
    auto black_piece = board.BitBoard(-piece);
    while (black_piece) {
      int index = Lsb1(black_piece);
      black_piece_stats[index]++;
      black_piece ^= (1ULL << index);
    }
  }
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; ++j) {
      std::cout << white_piece_stats[i * 8 + j] << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; ++j) {
      std::cout << black_piece_stats[i * 8 + j] << ", ";
    }
    std::cout << std::endl;
  }
}

void main_pawn_struct_tuning_loop() {
  static constexpr int batch_size = 1024;
  auto epd_records = Parse();
  std::cout << "Records: " << epd_records.size() << std::endl;
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(epd_records.begin(), epd_records.end(), g);

  std::vector<EPDRecord> train_records, test_records;
  int num_train_records = int(epd_records.size() * 0.9);
  for (int i = 0; i < num_train_records; ++i) {
    train_records.push_back(epd_records[i]);
  }
  for (size_t i = num_train_records; i < epd_records.size(); ++i) {
    test_records.push_back(epd_records[i]);
  }
  epd_records.clear();
  std::cout << "Train records size: " << train_records.size() << std::endl;
  std::cout << "Test records size: " << test_records.size() << std::endl;

  MLP<Variable> mlp(128, {12, 6, 3, 1});
  std::cout << "Network size (params): " << mlp.Parameters().size()
            << std::endl;
  auto parameters = mlp.Parameters();
  Parameters params;
  for (size_t i = 0; i < mlp.Parameters().size(); ++i) {
    params.Add(parameters.at(i));
  }

  // LogMetric(0, 0, "train_loss", AvgPawnStructLoss(train_records, mlp));
  // LogMetric(0, 0, "test_loss", AvgPawnStructLoss(test_records, mlp));

  std::vector<Variable> losses;
  int step = 0;
  int log_step = 0;
  for (int epoch = 0; epoch < 1000; ++epoch) {
    for (const auto& record : train_records) {
      Board board(Variant::STANDARD, record.fen);
      std::vector<U64> bbs = {board.BitBoard(PAWN), board.BitBoard(-PAWN)};
      auto val = mlp(bbs)[0];
      auto loss = PawnStructLoss(val, record.result);
      losses.push_back(loss);
      if (losses.size() >= batch_size) {
        step++;
        log_step++;
        auto avg_loss = losses.at(0);
        for (size_t i = 1; i < losses.size(); ++i) {
          avg_loss += losses.at(i);
        }
        avg_loss = avg_loss / losses.size();
        LogMetric(epoch, step, "batch_loss", double(avg_loss));
        params.ZeroGrad();
        avg_loss.Backward();
        params.ApplyMomentum();
        LogMetric(epoch, step, "grad_norm", params.GradNorm());
        params.ApplyGrad(1.0);
        LogMetric(epoch, step, "weights", params.WeightsMag());
        losses.clear();
      }
      if (log_step >= 250) {
        log_step = 0;
        LogMetric(epoch, step, "train_loss",
                  AvgPawnStructLoss(train_records, mlp));
        LogMetric(epoch, step, "test_loss",
                  AvgPawnStructLoss(test_records, mlp));
        std::cout << "MLP Parameters: ";
        for (const auto& param : mlp.Parameters()) {
          std::cout << param << ", ";
        }
        std::cout << std::endl;
      }
    }
  }
}

// best multiplier 1 / 136.2 -> loss 0.177305
void main_pawn_struct_eval_scoring_avg_loss() {
  auto epd_records = Parse();
  std::cout << "Records: " << epd_records.size() << std::endl;
  const StdEvalParams<double> eval_params = Params20241117Epoch99Step63720();

  double best_loss = 100000.;
  double best_d = -1.0;

  for (double d = 136.2; d <= 140.0; d += 0.01) {
    double loss = 0.0;
    for (auto& record : epd_records) {
      Board board(Variant::STANDARD, record.fen);
      auto full_eval_score =
          standard::StaticEval<double, false, false>(eval_params, board);
      auto partial_eval_score =
          standard::StaticEval<double, false, false, false>(eval_params, board);
      auto pawn_struct_score = full_eval_score - partial_eval_score;
      double val = 1.0 / (1.0 + exp(-pawn_struct_score * (1 / d)));
      val = record.result - val;
      loss += (val * val);
    }
    loss /= epd_records.size();
    if (loss < best_loss) {
      best_loss = loss;
      best_d = d;
      std::cout << "best_loss: " << best_loss << ", " << "best_d: " << best_d
                << std::endl;
    } else {
      std::cout << "loss: " << loss << std::endl;
    }
  }
}

void main_tuning_loop() {
  StdEvalParams<Variable> eval_params = Convert<double, Variable>(ExpTempo202502Epoch4Step2000Dbl());
=======
int main() {
  StdEvalParams<Variable> eval_params =
      Convert<double, Variable>(BlessedParamsDbl());
>>>>>>> master
  //StdEvalParams<Variable> eval_params = ZeroParams<Variable>();
  Parameters params = AsParameters(eval_params);

  auto epd_records = Parse();
  std::cout << "Records: " << epd_records.size() << std::endl;

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(epd_records.begin(), epd_records.end(), g);

  std::vector<EPDRecord> train_records, test_records;
  int num_train_records = int(epd_records.size() * 0.9);
  for (int i = 0; i < num_train_records; ++i) {
    train_records.push_back(epd_records[i]);
  }
  for (size_t i = num_train_records; i < epd_records.size(); ++i) {
    test_records.push_back(epd_records[i]);
  }
  epd_records.clear();
  std::cout << "Train records size: " << train_records.size() << std::endl;
  std::cout << "Test records size: " << test_records.size() << std::endl;

  //  std::cout << "Train Loss (step 0): " << AvgLoss(train_records,
  //  double_params)
  //            << std::endl;
  //  std::cout << "Test Loss (step 0): " << AvgLoss(test_records,
  //  double_params)
  //            << std::endl;

  std::vector<Variable> losses;
  int step = 0;
  int log_step = 0;
  int epoch = 0;
  do {
    epoch++;
    for (const auto& record : train_records) {
      Board board(Variant::STANDARD, record.fen);
<<<<<<< HEAD
      auto score = standard::StaticEval<Variable, false, false>(eval_params, board);
=======
      auto score =
          standard::StaticEval<Variable, false, false>(eval_params, board);
>>>>>>> master
      {
        auto loss = Loss(score, record.result, eval_params);
        losses.push_back(loss);
      }
      if (losses.size() >= kBatchSize) {
        step++;
        log_step++;
        auto avg_loss = losses.at(0);
        for (size_t i = 1; i < losses.size(); ++i) {
          avg_loss += losses.at(i);
        }
        avg_loss = avg_loss / losses.size();
        LogMetric(epoch, step, "batch_loss", double(avg_loss));
        params.ZeroGrad();
        avg_loss.Backward();
        params.ApplyMomentum();
        LogMetric(epoch, step, "grad_norm", params.GradNorm());
        params.ApplyGrad(kLearningRate);
        LogMetric(epoch, step, "weights", params.WeightsMag());
        losses.clear();
      }
      if (log_step >= 50) {
        log_step = 0;
        auto double_params = Convert<Variable, double>(eval_params);
        WriteFile(double_params, epoch, step);
        LogMetric(epoch, step, "train_loss",
                  AvgLoss(train_records, double_params));
        LogMetric(epoch, step, "test_loss",
                  AvgLoss(test_records, double_params));
        auto int_params = Convert<double, int>(double_params);
        LogMetric(epoch, step, "integerized_train_loss",
                  AvgLoss(train_records, int_params));
        LogMetric(epoch, step, "integerized_test_loss",
                  AvgLoss(test_records, int_params));
      }
    }
  } while (epoch + 1 <= kMaxEpochs);

  {
    auto double_params = Convert<Variable, double>(eval_params);
    std::cout << "Final Train Loss: " << AvgLoss(train_records, double_params)
              << std::endl;
    std::cout << "Final Test Loss: " << AvgLoss(test_records, double_params)
              << std::endl;
    auto int_params = Convert<double, int>(double_params);
    std::cout << "Final Train Loss (integerized): "
              << AvgLoss(train_records, int_params) << std::endl;
    std::cout << "Final Test Loss (integerized): "
              << AvgLoss(test_records, int_params) << std::endl;
    WriteFile(double_params, epoch, step, "Final");
  }
}

int main() {
  main_tuning_loop();
  // main_find_best_multiplier();
  // main_piece_occupancy_stats();
  // main_pawn_struct_tuning_loop();
  //main_pawn_struct_eval_scoring_avg_loss();
  return 0;
}
