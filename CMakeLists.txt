cmake_minimum_required(VERSION 3.15)
project(nakshatra CXX)

if(LOWMEM)
  set(TRANSPOS_SIZE "(1 << 21)")
else()
  set(TRANSPOS_SIZE "(1 << 24)")
endif()

add_definitions(
    -DENGINE_NAME="Nakshatra"
    -DANTICHESS_EGTB_PATH_GLOB="${CMAKE_BINARY_DIR}/egtb/*.egtb"
    -DSTANDARD_TRANSPOS_SIZE=${TRANSPOS_SIZE}
    -DANTICHESS_TRANSPOS_SIZE=${TRANSPOS_SIZE}
    -DNUM_THREADS=1
)

if(SAVETIME)
  add_definitions(-DSAVETIME=1)
endif()

include(FetchContent)

FetchContent_Declare(
    magic_bits
    GIT_REPOSITORY https://github.com/goutham/magic-bits.git
    GIT_TAG master
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/magic-bits"
)
FetchContent_GetProperties(magic_bits)
if(NOT magic_bits_POPULATED)
  FetchContent_Populate(magic_bits)
  include_directories(${FETCHCONTENT_BASE_DIR})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-flto -O3 -Wall -march=native")
include_directories("src")

#
# Libraries
#

add_library(common 
    src/common.cpp
    src/fen.cpp
)

add_library(board
    src/board.cpp
    src/zobrist.cpp
)

add_library(movegen
    src/movegen.cpp
    src/see.cpp
    src/attacks.cpp
    src/san.cpp
)

add_library(evaluator
    src/eval_antichess.cpp
    src/eval_standard.cpp
    src/egtb.cpp
    src/pawns.cpp
)

add_library(player
    src/player.cpp
    src/pn_search.cpp
    src/id_search.cpp
    src/move_order.cpp
    src/pv_search.cpp
    src/transpos.cpp
)

add_library(executor
    src/executor.cpp
)

add_library(egtb_gen
    src/egtb_gen.cpp
)

#
# Executables
#

add_executable(nakshatra
    src/nakshatra.cpp
)
target_link_libraries(nakshatra 
    executor player evaluator movegen board common pthread
)

add_executable(pns_analyze
    src/pns_analyze.cpp
)
target_link_libraries(pns_analyze
    player evaluator movegen board common
)

add_executable(movegen_perf
    src/movegen_perf.cpp
)
target_link_libraries(movegen_perf
    movegen board common
)

add_executable(egtb_gen_main
    src/egtb_gen_main.cpp
)
target_link_libraries(egtb_gen_main
    egtb_gen evaluator movegen board common
)

#
# Tests
#

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_FLAGS_TEST "-std=c++20")
file(GLOB TEST_SOURCES "src/tests/*_test.cpp")

add_executable(unit_tests
    ${TEST_SOURCES}
)
target_link_libraries(unit_tests
    gtest gtest_main egtb_gen executor player evaluator movegen board common pthread
)
