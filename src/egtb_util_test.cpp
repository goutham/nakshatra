#include "board.h"
#include "common.h"
#include "egtb_util.h"
#include "fen.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(EGTBUtilTest, VerifyUtil) {
  std::string fen1 = "2Q5/8/8/8/1k6/8/p7/8 w - -";
  char* eb = EGTBUtil::ConvertFENToByteArray(fen1);
  std::string fen2 = EGTBUtil::ConvertByteArrayToFEN(eb);
  EXPECT_EQ(fen1, fen2);
}

