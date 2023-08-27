#!/bin/bash

if test ! -f CMakeLists.txt; then
  cat >&2 << __EOF__
Must run this script from the directory containing CMakeLists.txt
__EOF__
exit 1
fi

mkdir -p build
cd build
cmake -GNinja -DLOWMEM=OFF -DSAVETIME=OFF ..
ninja

mkdir -p egtb
echo "Generating Antichess EGTB..."
./egtb_gen_main

echo "Running unit tests..."
./unit_tests
