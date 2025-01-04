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
#include <string>

const std::string kDataFile = "TODO";
constexpr double kMultiplier = 1.0 / 113.6;
constexpr double kLearningRate = 10.0;
constexpr int kBatchSize = 1024;

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
inline void PrintParams(const StdEvalParams<ValueType>& params) {
  std::cout << "static constexpr StdEvalParams<double> double_params{"
            << std::endl;
  std::cout << "  .pv_mgame = {";
  for (size_t i = 0; i < params.pv_mgame.size(); ++i) {
    std::cout << params.pv_mgame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .pv_egame = {";
  for (size_t i = 0; i < params.pv_egame.size(); ++i) {
    std::cout << params.pv_egame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .pst_mgame = {";
  for (size_t i = 0; i < params.pst_mgame.size(); ++i) {
    std::cout << "std::array<double, 64>{";
    for (size_t j = 0; j < params.pst_mgame[i].size(); ++j) {
      std::cout << params.pst_mgame[i][j] << ", ";
    }
    std::cout << "}," << std::endl;
  }
  std::cout << "}," << std::endl;

  std::cout << "  .pst_egame = {";
  for (size_t i = 0; i < params.pst_egame.size(); ++i) {
    std::cout << "std::array<double, 64>{";
    for (size_t j = 0; j < params.pst_egame[i].size(); ++j) {
      std::cout << params.pst_egame[i][j] << ", ";
    }
    std::cout << "}," << std::endl;
  }
  std::cout << "}," << std::endl;
  std::cout << "  .doubled_pawns_mgame = {";
  for (size_t i = 0; i < params.doubled_pawns_mgame.size(); ++i) {
    std::cout << params.doubled_pawns_mgame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .doubled_pawns_egame = {";
  for (size_t i = 0; i < params.doubled_pawns_egame.size(); ++i) {
    std::cout << params.doubled_pawns_egame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .passed_pawns_mgame = {";
  for (size_t i = 0; i < params.passed_pawns_mgame.size(); ++i) {
    std::cout << params.passed_pawns_mgame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .passed_pawns_egame = {";
  for (size_t i = 0; i < params.passed_pawns_egame.size(); ++i) {
    std::cout << params.passed_pawns_egame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .isolated_pawns_mgame = {";
  for (size_t i = 0; i < params.isolated_pawns_mgame.size(); ++i) {
    std::cout << params.isolated_pawns_mgame[i] << ", ";
  }
  std::cout << "}," << std::endl;
  std::cout << "  .isolated_pawns_egame = {";
  for (size_t i = 0; i < params.isolated_pawns_egame.size(); ++i) {
    std::cout << params.isolated_pawns_egame[i] << ", ";
  }
  std::cout << "}," << std::endl;

  std::cout << "  .mobility_mgame = {";
  for (size_t i = 0; i < params.mobility_mgame.size(); ++i) {
    std::cout << "std::array<double, 64>{";
    for (size_t j = 0; j < params.mobility_mgame[i].size(); ++j) {
      std::cout << params.mobility_mgame[i][j] << ", ";
    }
    std::cout << "}," << std::endl;
  }
  std::cout << "}," << std::endl;

  std::cout << "  .mobility_egame = {";
  for (size_t i = 0; i < params.mobility_egame.size(); ++i) {
    std::cout << "std::array<double, 64>{";
    for (size_t j = 0; j < params.mobility_egame[i].size(); ++j) {
      std::cout << params.mobility_egame[i][j] << ", ";
    }
    std::cout << "}," << std::endl;
  }
  std::cout << "}," << std::endl;
  std::cout << "};" << std::endl;
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
  for (int epoch = 0; epoch < 100; ++epoch) {
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
        PrintParams(double_params);
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
  }

  {
    auto double_params = Convert<Variable, double>(eval_params);
    std::cout << "Final Train Loss: " << AvgLoss(train_records, double_params)
              << std::endl;
    std::cout << "Final Test Loss: " << AvgLoss(test_records, double_params)
              << std::endl;
    PrintParams(double_params);

    auto int_params = Convert<double, int>(double_params);
    std::cout << "Final Train Loss (integerized): "
              << AvgLoss(train_records, int_params) << std::endl;
    std::cout << "Final Test Loss (integerized): "
              << AvgLoss(test_records, int_params) << std::endl;
    PrintParams(int_params);
  }

  return 0;
}
