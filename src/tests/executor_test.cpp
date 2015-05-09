#include "common.h"
#include "board.h"
#include "executor.h"
#include "movegen.h"
#include "zobrist.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

class ExecutorTest : public testing::Test {
 public:
  ExecutorTest() {
    zobrist::InitializeIfNeeded();
    movegen::InitializeIfNeeded();
  }
};

void PrintResponse(const vector<string>& response) {
  for (const string& s : response) {
    std::cout << s << std::endl;
  }
}

TEST_F(ExecutorTest, VerifyResultOnGameEnd) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 w - -";
  Executor executor("nakshatra-test", fen, SUICIDE);
  vector<string> response;
  executor.Execute("new", &response);
  executor.Execute("variant suicide", &response);
  executor.Execute("usermove b5b6", &response);
  EXPECT_EQ(1, response.size());
  EXPECT_EQ("0-1 {Black Wins}", response.at(0));
}

TEST_F(ExecutorTest, VerifyResultOnGameEnd2) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 b - -";
  Executor executor("nakshatra-test", fen, SUICIDE);
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

TEST_F(ExecutorTest, VerifyResult) {
  const string fen = "8/8/1k6/1K6/8/8/8/8 w - -";
  Executor executor("nakshatra-test", fen, SUICIDE);
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
