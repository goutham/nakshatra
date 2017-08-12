#include "common.h"
#include "glob.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

std::ostream nullstream(0);

std::vector<string> SplitString(const string& s, char delim) {
  std::vector<string> vec;
  std::istringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    vec.push_back(item);
  }
  return vec;
}

int StringToInt(const string& s) {
  std::istringstream ss(s);
  int i;
  ss >> i;
  return i;
}

string LongToString(long l) {
  std::stringstream ss;
  ss << l;
  return ss.str();
}

// clang-format off
const int index64[64] = {
   63,  0, 58,  1, 59, 47, 53,  2,
   60, 39, 48, 27, 54, 33, 42,  3,
   61, 51, 37, 40, 49, 18, 28, 20,
   55, 30, 34, 11, 43, 14, 22,  4,
   62, 57, 46, 52, 38, 26, 32, 41,
   50, 36, 17, 19, 29, 10, 13, 21,
   56, 45, 25, 31, 35, 16,  9, 12,
   44, 24, 15,  8, 23,  7,  6,  5
};
// clang-format on

/**
 * bitScanForward
 * @author Charles E. Leiserson
 *         Harald Prokop
 *         Keith H. Randall
 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
int log2U(U64 bb) {
  const U64 debruijn64 = 0x07EDD5E59A4E28C2ULL;
  assert(bb != 0ULL);
  return index64[((bb & -bb) * debruijn64) >> 58];
}

unsigned PopCount(U64 x) {
  unsigned count = 0;
  while (x) {
    ++count;
    x &= (x - 1);
  }
  return count;
}

bool GlobFiles(const string& regex, vector<string>* filenames) {
  glob_t globbuf;
  int err = glob(regex.c_str(), 0, NULL, &globbuf);
  if (err != 0) {
    return false;
  }
  for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
    filenames->push_back(globbuf.gl_pathv[i]);
  }
  globfree(&globbuf);
  return true;
}
