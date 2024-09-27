#include "common.h"
#include "glob.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

std::ostream nullstream(0);

std::vector<std::string> SplitString(const std::string& s, char delim) {
  std::vector<std::string> vec;
  std::istringstream ss(s);
  for (std::string item; getline(ss, item, delim);) {
    vec.push_back(item);
  }
  return vec;
}

int StringToInt(const std::string& s) {
  std::istringstream ss(s);
  int i;
  ss >> i;
  return i;
}

std::string LongToString(long l) {
  std::stringstream ss;
  ss << l;
  return ss.str();
}

bool GlobFiles(const std::string& regex, std::vector<std::string>* filenames) {
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
