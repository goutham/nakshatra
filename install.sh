#!/bin/bash

if test ! -f CMakeLists.txt; then
  cat >&2 << __EOF__
Must run this script from the directory containing CMakeLists.txt
__EOF__
exit 1
fi

if test ! -e build; then
  mkdir build
  cd build
  cmake -GNinja -DLOWMEM=OFF -DSAVETIME=OFF ..
  ninja

  mkdir egtb
  echo "Generating Antichess EGTB..."
  ./egtb_gen_main

  echo "Running unit tests..."
  ./unit_tests
else
  echo "To reinstall, delete build/ directory before running this script."
  echo "For incremental builds, run cmake / ninja from the build/ directory."
fi
