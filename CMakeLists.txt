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
if(DEBUG)
  set(CMAKE_CXX_FLAGS "-Wall")
else()
  set(CMAKE_CXX_FLAGS "-flto=auto -O3 -Wall -march=native")
endif()
include_directories("src")

#
# Libraries
#

set(CORE_LIBS
    attacks
    board
    common
    egtb
    eval_antichess
    eval_standard
    executor
    fen
    id_search
    move_order
    movegen
    pawns
    player
    pn_search
    pv_search
    san
    see
    transpos
    zobrist
)
foreach(CORE_LIB ${CORE_LIBS})
    add_library(${CORE_LIB} OBJECT "src/${CORE_LIB}.cpp")
endforeach()

add_library(egtb_gen OBJECT src/egtb_gen.cpp)

#
# Executables
#

add_executable(nakshatra src/nakshatra.cpp)
target_link_libraries(nakshatra ${CORE_LIBS} pthread)

if(BUILD_ALL_EXECUTABLES)
    add_executable(pns_analyze src/pns_analyze.cpp)
    target_link_libraries(pns_analyze ${CORE_LIBS})

    add_executable(movegen_perf src/movegen_perf.cpp)
    target_link_libraries(movegen_perf ${CORE_LIBS})

    add_executable(egtb_gen_main src/egtb_gen_main.cpp)
    target_link_libraries(egtb_gen_main ${CORE_LIBS} egtb_gen)
endif()

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
    gtest gtest_main ${CORE_LIBS} pthread
)
