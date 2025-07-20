#include <gtest/gtest.h>
#include <string>
#include "main.h"
#include "uci_executor.h"

// Test the protocol detection logic
class UCIProtocolTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UCIProtocolTest, DetectXBoardByDefault) {
  const char* argv[] = {"nakshatra"};
  int argc = 1;
  char* argv_copy[] = {const_cast<char*>(argv[0])};
  
  Protocol result = DetectProtocol(argc, argv_copy);
  EXPECT_EQ(result, Protocol::XBOARD);
}

TEST_F(UCIProtocolTest, DetectUCIWithFlag) {
  const char* argv[] = {"nakshatra", "--uci"};
  int argc = 2;
  char* argv_copy[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1])};
  
  Protocol result = DetectProtocol(argc, argv_copy);
  EXPECT_EQ(result, Protocol::UCI);
}

TEST_F(UCIProtocolTest, DetectUCIWithOtherArgs) {
  const char* argv[] = {"nakshatra", "--debug", "--uci", "--verbose"};
  int argc = 4;
  char* argv_copy[] = {
    const_cast<char*>(argv[0]), 
    const_cast<char*>(argv[1]), 
    const_cast<char*>(argv[2]), 
    const_cast<char*>(argv[3])
  };
  
  Protocol result = DetectProtocol(argc, argv_copy);
  EXPECT_EQ(result, Protocol::UCI);
}

TEST_F(UCIProtocolTest, DetectXBoardWithOtherArgs) {
  const char* argv[] = {"nakshatra", "--debug", "--verbose"};
  int argc = 3;
  char* argv_copy[] = {
    const_cast<char*>(argv[0]), 
    const_cast<char*>(argv[1]), 
    const_cast<char*>(argv[2])
  };
  
  Protocol result = DetectProtocol(argc, argv_copy);
  EXPECT_EQ(result, Protocol::XBOARD);
}

// Test the UCI executor
class UCIExecutorTest : public ::testing::Test {
protected:
  void SetUp() override {
    executor = std::make_unique<UCIExecutor>();
  }
  void TearDown() override {}
  
  std::unique_ptr<UCIExecutor> executor;
};

TEST_F(UCIExecutorTest, InitialState) {
  EXPECT_FALSE(executor->quit());
}

TEST_F(UCIExecutorTest, UCICommand) {
  auto response = executor->Execute("uci");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_FALSE(response.empty());
  
  // Should contain engine identification
  bool found_name = false;
  bool found_author = false;
  bool found_uciok = false;
  
  for (const auto& line : response) {
    if (line.find("id name") != std::string::npos) {
      found_name = true;
    }
    if (line.find("id author") != std::string::npos) {
      found_author = true;
    }
    if (line == "uciok") {
      found_uciok = true;
    }
  }
  
  EXPECT_TRUE(found_name);
  EXPECT_TRUE(found_author);
  EXPECT_TRUE(found_uciok);
}

TEST_F(UCIExecutorTest, IsReadyCommand) {
  auto response = executor->Execute("isready");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_EQ(response.size(), 1);
  EXPECT_EQ(response[0], "readyok");
}

TEST_F(UCIExecutorTest, QuitCommand) {
  EXPECT_FALSE(executor->quit());
  
  auto response = executor->Execute("quit");
  
  EXPECT_TRUE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, UnknownCommand) {
  auto response = executor->Execute("unknown_command");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, EmptyCommand) {
  auto response = executor->Execute("");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, UCINewGameCommand) {
  auto response = executor->Execute("ucinewgame");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionStartpos) {
  auto response = executor->Execute("position startpos");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, InvalidPositionCommand) {
  auto response = executor->Execute("position");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionSequence) {
  // Test sequence: ucinewgame followed by position startpos
  auto response1 = executor->Execute("ucinewgame");
  auto response2 = executor->Execute("position startpos");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response1.empty());
  EXPECT_TRUE(response2.empty());
}

TEST_F(UCIExecutorTest, GoInfiniteCommand) {
  auto response = executor->Execute("go infinite");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoWithoutInfinite) {
  auto response = executor->Execute("go");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, StopCommand) {
  auto response = executor->Execute("stop");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionFENValid) {
  // Standard starting position in FEN
  auto response = executor->Execute("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionFENMiddlegame) {
  // A middlegame position
  auto response = executor->Execute("position fen r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionFENInvalid) {
  // Invalid FEN - should be handled gracefully
  auto response = executor->Execute("position fen invalid_fen_string");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionFENIncomplete) {
  // Incomplete FEN command
  auto response = executor->Execute("position fen rnbqkbnr/pppppppp");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionStartposMoves) {
  // Test position with move sequence
  auto response = executor->Execute("position startpos moves e2e4 e7e5");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionFENMoves) {
  // Test FEN position with moves
  auto response = executor->Execute("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4 e7e5 g1f3");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionMovesLongSequence) {
  // Test longer move sequence (Ruy Lopez opening)
  auto response = executor->Execute("position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionInvalidMove) {
  // Test with invalid move in sequence
  auto response = executor->Execute("position startpos moves e2e4 e7e5 invalid_move");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, PositionMovesWithoutKeyword) {
  // Test position without "moves" keyword
  auto response = executor->Execute("position startpos e2e4 e7e5");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoMovetime) {
  auto response = executor->Execute("go movetime 500");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoDepth) {
  auto response = executor->Execute("go depth 5");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoInvalidMovetime) {
  auto response = executor->Execute("go movetime invalid");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoInvalidDepth) {
  auto response = executor->Execute("go depth invalid");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, GoNoParameters) {
  auto response = executor->Execute("go");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, UCICommandShowsOptions) {
  auto response = executor->Execute("uci");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_FALSE(response.empty());
  
  // Should contain engine options
  bool found_hash = false;
  bool found_variant = false;
  
  for (const auto& line : response) {
    if (line.find("option name Hash") != std::string::npos) {
      found_hash = true;
    }
    if (line.find("option name UCI_Variant") != std::string::npos) {
      found_variant = true;
    }
  }
  
  EXPECT_TRUE(found_hash);
  EXPECT_TRUE(found_variant);
}

TEST_F(UCIExecutorTest, SetOptionHash) {
  auto response = executor->Execute("setoption name Hash value 64");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, SetOptionUCIVariantSuicide) {
  auto response = executor->Execute("setoption name UCI_Variant value suicide");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, SetOptionUCIVariantStandard) {
  auto response = executor->Execute("setoption name UCI_Variant value standard");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, SetOptionInvalidFormat) {
  auto response = executor->Execute("setoption invalid format");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, SetOptionInvalidHash) {
  auto response = executor->Execute("setoption name Hash value invalid");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

TEST_F(UCIExecutorTest, SetOptionUnknownOption) {
  auto response = executor->Execute("setoption name UnknownOption value test");
  
  EXPECT_FALSE(executor->quit());
  EXPECT_TRUE(response.empty());
}

