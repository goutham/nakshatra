#include "board.h"
#include "common.h"
#include "params/params.h"
#include "std_static_eval.h"
#include "tuning/parameters.h"
#include "tuning/variable.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

const std::string kExperimentName = "Params";
const std::string kDataFile = "";
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
inline StdEvalParams<ToType> Convert(const StdEvalParams<FromType>& fparams) {
  StdEvalParams<ToType> params;
  for (size_t i = 0; i < fparams.pv_mgame.size(); ++i) {
    params.pv_mgame[i] = ToType(fparams.pv_mgame[i]);
  }
  for (size_t i = 0; i < fparams.pv_egame.size(); ++i) {
    params.pv_egame[i] = ToType(fparams.pv_egame[i]);
  }
  for (size_t i = 0; i < fparams.pst_mgame.size(); ++i) {
    for (size_t j = 0; j < fparams.pst_mgame[i].size(); ++j) {
      params.pst_mgame[i][j] = ToType(fparams.pst_mgame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.pst_egame.size(); ++i) {
    for (size_t j = 0; j < fparams.pst_egame[i].size(); ++j) {
      params.pst_egame[i][j] = ToType(fparams.pst_egame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.doubled_pawns_mgame.size(); ++i) {
    params.doubled_pawns_mgame[i] = ToType(fparams.doubled_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.doubled_pawns_egame.size(); ++i) {
    params.doubled_pawns_egame[i] = ToType(fparams.doubled_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.passed_pawns_mgame.size(); ++i) {
    params.passed_pawns_mgame[i] = ToType(fparams.passed_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.passed_pawns_egame.size(); ++i) {
    params.passed_pawns_egame[i] = ToType(fparams.passed_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.isolated_pawns_mgame.size(); ++i) {
    params.isolated_pawns_mgame[i] = ToType(fparams.isolated_pawns_mgame[i]);
  }
  for (size_t i = 0; i < fparams.isolated_pawns_egame.size(); ++i) {
    params.isolated_pawns_egame[i] = ToType(fparams.isolated_pawns_egame[i]);
  }
  for (size_t i = 0; i < fparams.mobility_mgame.size(); ++i) {
    for (size_t j = 0; j < fparams.mobility_mgame[i].size(); ++j) {
      params.mobility_mgame[i][j] = ToType(fparams.mobility_mgame[i][j]);
    }
  }
  for (size_t i = 0; i < fparams.mobility_egame.size(); ++i) {
    for (size_t j = 0; j < fparams.mobility_egame[i].size(); ++j) {
      params.mobility_egame[i][j] = ToType(fparams.mobility_egame[i][j]);
    }
  }
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
  const std::string param_type = GetParamType<ValueType>();
  ofs << "  ." << name << " = {";
  for (size_t i = 0; i < array.size(); ++i) {
    ofs << array[i] << ", ";
  }
  ofs << "}," << std::endl;
}

template <typename ValueType>
void WriteFunction(const StdEvalParams<ValueType>& params,
                   const std::string exp_name, std::ofstream& ofs) {
  const std::string param_type = GetParamType<ValueType>();
  const std::string fn_suffix = GetFnSuffix<ValueType>();
  const std::string fn_name = exp_name + fn_suffix;
  const std::string array_type_64 = "std::array<" + param_type + ", 64>";
  ofs << "inline StdEvalParams<" << param_type << ">" << fn_name << "() {"
      << std::endl;
  ofs << "static constexpr StdEvalParams<" << param_type << "> params{"
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
  WriteParams(ofs, "mobility_mgame", params.mobility_mgame);
  WriteParams(ofs, "mobility_egame", params.mobility_egame);

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

template <typename ValueType>
double AvgLoss(const std::vector<EPDRecord>& epd_records,
               const StdEvalParams<ValueType>& params,
               double multiplier = kMultiplier) {
  double loss = 0.0;
  for (const auto& record : epd_records) {
    Board board(Variant::STANDARD, record.fen);
    const double score = standard::StaticEval<ValueType, false>(params, board);
    loss += double(Loss(score, record.result, params, multiplier));
  }
  return loss / epd_records.size();
}

void LogMetric(int epoch, int step, const std::string& metric, double value) {
  std::cout << "epoch:" << epoch << ", step:" << step << ", metric:" << metric
            << ", value:" << value << std::endl;
}

/*
// best multiplier = 1 / 113.6
int _main() {
  StdEvalParams<double> eval_params = Convert<int, double>(OrigParams());
  auto epd_records = Parse();
  double best_loss = 100000.;
  double best_d = -1.0;
  for (double d = 112.0; d <= 115.0; d += 0.1) {
    double loss = 0.0;
    for (auto& record : epd_records) {
      Board board(Variant::STANDARD, record.fen);
      auto score = standard::StaticEval<double, false>(eval_params, board);
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
 return 0;
}
*/

int main() {
  StdEvalParams<Variable> eval_params =
      Convert<int, Variable>(MobilityParamsEpoch13Step8500());
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
      auto score = standard::StaticEval<Variable, false>(eval_params, board);
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

  return 0;
}
