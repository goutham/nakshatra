cmake_minimum_required(VERSION 3.15)
project(nakshatra CXX)

if(LOWMEM)
  set(TRANSPOS_SIZE "(1 << 21)")
else()
  set(TRANSPOS_SIZE "(1 << 24)")
endif()
set(NUM_THREADS
    "1"
    CACHE STRING "Number of threads")

add_definitions(
  -DENGINE_NAME="Nakshatra" -DSTANDARD_TRANSPOS_SIZE=${TRANSPOS_SIZE}
  -DANTICHESS_TRANSPOS_SIZE=${TRANSPOS_SIZE} -DNUM_THREADS=${NUM_THREADS})

include(FetchContent)

FetchContent_Declare(
  magic_bits
  GIT_REPOSITORY https://github.com/goutham/magic-bits.git
  GIT_TAG master
  SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/magic-bits")
FetchContent_GetProperties(magic_bits)
if(NOT magic_bits_POPULATED)
  FetchContent_Populate(magic_bits)
  include_directories(${FETCHCONTENT_BASE_DIR})
endif()

set(CMAKE_CXX_STANDARD 20)
if(DEBUG)
  set(CMAKE_CXX_FLAGS "-Wall")
else()
  set(CMAKE_CXX_FLAGS "-flto=auto -O3 -Wall -march=native")
endif()
include_directories("src")

#
# Libraries
#

set(SOURCES
    src/attacks.cpp
    src/board.cpp
    src/common.cpp
    src/egtb.cpp
    src/eval_antichess.cpp
    src/eval_standard.cpp
    src/executor.cpp
    src/fen.cpp
    src/id_search.cpp
    src/move_order.cpp
    src/movegen.cpp
    src/player.cpp
    src/pn_search.cpp
    src/pv_search.cpp
    src/san.cpp
    src/see.cpp
    src/transpos.cpp
    src/zobrist.cpp)
add_library(nakshatra_core OBJECT ${SOURCES})

#
# Executables
#

add_executable(nakshatra src/nakshatra.cpp)
target_link_libraries(nakshatra nakshatra_core pthread)

if(BUILD_ALL_EXECUTABLES)
  add_executable(pns_analyze src/pns_analyze.cpp)
  target_link_libraries(pns_analyze nakshatra_core)

  add_executable(movegen_perf src/movegen_perf.cpp)
  target_link_libraries(movegen_perf nakshatra_core)

  add_executable(tune src/tuning/tune.cpp)
  target_link_libraries(tune nakshatra_core)
endif()

#
# Tests
#

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_FLAGS_TEST "-std=c++20")
file(GLOB TEST_SOURCES "src/tests/*_test.cpp")

add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests gtest gtest_main nakshatra_core pthread)
