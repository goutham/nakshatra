#!/bin/bash

set -e

if test ! -f CMakeLists.txt; then
  cat >&2 << __EOF__
Must run this script from the directory containing CMakeLists.txt
__EOF__
exit 1
fi

mkdir -p build
cd build
cmake .. -GNinja -DLOWMEM=OFF -DDEBUG=OFF -DBUILD_ALL_EXECUTABLES=ON
ninja

mkdir -p egtb
echo "Generating Antichess EGTB..."
./egtb_gen_main

echo "Running unit tests..."
./unit_tests

# Change cmake settings to not build all executables by default.
echo "Updating cmake default settings..."
cmake .. -DBUILD_ALL_EXECUTABLES=OFF
