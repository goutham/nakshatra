cmake_minimum_required(VERSION 3.15)
project(nakshatra CXX)

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
include_directories(".")

#
# Libraries
#

add_library(common 
    common.cpp
    fen.cpp
)

add_library(board
    board.cpp
    zobrist.cpp
)

add_library(movegen
    movegen.cpp
    see.cpp
    attacks.cpp
    san.cpp
)

add_library(evaluator
    eval_antichess.cpp
    eval_standard.cpp
    egtb.cpp
    pawns.cpp
)

add_library(player
    player.cpp
    pn_search.cpp
    id_search.cpp
    move_order.cpp
    pv_search.cpp
    transpos.cpp
)

add_library(executor
    executor.cpp
)

add_library(egtb_gen
    egtb_gen.cpp
)

#
# Executables
#

add_executable(nakshatra
    nakshatra.cpp
)
target_link_libraries(nakshatra 
    executor player evaluator movegen board common pthread
)

add_executable(pns_analyze
    pns_analyze.cpp
)
target_link_libraries(pns_analyze
    player evaluator movegen board common
)

add_executable(movegen_perf
    movegen_perf.cpp
)
target_link_libraries(movegen_perf
    movegen board common
)

add_executable(egtb_gen_main
    egtb_gen_main.cpp
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
file(GLOB TEST_SOURCES "tests/*_test.cpp")

add_executable(unit_tests
    ${TEST_SOURCES}
)
target_link_libraries(unit_tests
    gtest gtest_main egtb_gen executor player evaluator movegen board common pthread
)
