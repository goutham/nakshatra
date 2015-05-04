#ifndef EGTB_UTIL_H
#define EGTB_UTIL_H

#include "common.h"

#include <string>

class EGTBUtil {
 public:
  static char* ConvertFENToByteArray(const std::string& fen);
  static std::string ConvertByteArrayToFEN(char* b_array);

 private:
  EGTBUtil() { }
};

#endif
