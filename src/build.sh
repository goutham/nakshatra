#!/bin/bash
#
# Options to ./build.sh
#   - lowmem: lower mem usage mode.
#   - savetime: lower accuracy but saves time on some types of moves.

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

cat > config.h << __EOF__
#define ENGINE_NAME "Nakshatra"
__EOF__

for opt in "$@"
do
cat >> config.h << __EOF__
#define $(echo ${opt} | tr '[:lower:]' '[:upper:]')
__EOF__
done

cat >> config.h << __EOF__
#define ANTICHESS_EGTB_PATH_GLOB "${PWD}/egtb/*.egtb"

#ifdef LOWMEM
// 128 MB
#define STANDARD_TRANSPOS_SIZE (1 << 21)
// 128 MB
#define ANTICHESS_TRANSPOS_SIZE (1 << 21)
#else
// 1 GB
#define STANDARD_TRANSPOS_SIZE (1 << 24)
// 1 GB
#define ANTICHESS_TRANSPOS_SIZE (1 << 24)
#endif
__EOF__

cat >> config.h << __EOF__
#define NUM_THREADS 1
__EOF__

cmake -GNinja . && ninja

if test ! -e egtb; then
  mkdir egtb
  ./egtb_gen_main
fi

echo "Running unit tests..."
./unit_tests
