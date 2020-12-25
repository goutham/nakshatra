#include "board.h"
#include "common.h"
#include "executor.h"
#include "movegen.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

void PrintResponse(const vector<string>& response) {
  for (const string& s : response) {
    std::cout << s << std::endl;
  }
}

TEST(ExecutorTest, VerifyResultOnGameEnd_Antichess) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 w - -";
  Executor executor("nakshatra-test", fen, Variant::ANTICHESS);
  vector<string> response;
  executor.Execute("new", &response);
  executor.Execute("variant suicide", &response);
  executor.Execute("usermove b5b6", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("0-1 {Black Wins}", response.at(0));
}

TEST(ExecutorTest, VerifyResultOnGameEnd2_Antichess) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 b - -";
  Executor executor("nakshatra-test", fen, Variant::ANTICHESS);
  vector<string> response;
  executor.Execute("new", &response);
  executor.Execute("variant suicide", &response);
  executor.Execute("go", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("move b6b5", response.at(0));
  response.clear();
  executor.Execute("go", &response);
  EXPECT_EQ("1-0 {White Wins}", response.at(0));
}

TEST(ExecutorTest, VerifyResult_Antichess) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 w - -";
  Executor executor("nakshatra-test", fen, Variant::ANTICHESS);
  vector<string> response;
  executor.Execute("new", &response);
  executor.Execute("variant suicide", &response);
  executor.Execute("go", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("move b5b6", response.at(0));
  response.clear();
  executor.Execute("go", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("0-1 {Black Wins}", response.at(0));
}

TEST(ExecutorTest, VerifyResultWhiteWins_Standard) {
  const string fen = "8/7k/8/7R/6Q1/8/8/1K6 b - -";
  Executor executor("nakshatra-test", fen, Variant::STANDARD);
  vector<string> response;
  executor.Execute("go", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("1-0 {White Wins}", response.at(0));
}

TEST(ExecutorTest, VerifyResultBlackWins_Standard) {
  const string fen = "5k2/8/8/8/7R/2n5/8/K1q5 w - -";
  Executor executor("nakshatra-test", fen, Variant::STANDARD);
  vector<string> response;
  executor.Execute("go", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("0-1 {Black Wins}", response.at(0));
}
