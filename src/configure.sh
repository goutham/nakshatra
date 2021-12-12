#!/bin/bash

if test ! -f nakshatra.cpp; then
  cat >&2 << __EOF__
Must run this script from root of the distribution tree.
__EOF__
exit 1
fi

if test ! -e magic-bits; then
  echo "Fetching magic-bits library..."
  git clone https://github.com/goutham/magic-bits.git
fi

if test ! -e gtest; then
  echo "Fetching googletest..."
  curl -L -O https://github.com/google/googletest/archive/release-1.7.0.zip
  unzip -q release-1.7.0.zip
  rm release-1.7.0.zip
  mv googletest-release-1.7.0 gtest
fi

scons

if test ! -e egtb; then
  mkdir egtb
  ./egtb_gen_main
fi

echo "Running unit tests..."
./unit_tests
