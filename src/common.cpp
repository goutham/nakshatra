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
  for (string item; getline(ss, item, delim);) {
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

bool GlobFiles(const string& regex, vector<string>* filenames) {
  glob_t globbuf;
  if (int err = glob(regex.c_str(), 0, NULL, &globbuf); err != 0) {
    return false;
  }
  for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
    filenames->push_back(globbuf.gl_pathv[i]);
  }
  globfree(&globbuf);
  return true;
}
