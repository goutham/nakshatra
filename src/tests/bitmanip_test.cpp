#include "bitmanip.h"
#include "common.h"

#include <gtest/gtest.h>
#include <iostream>

TEST(BitManipTest, FromLayout) {
  const char* layout_str = R"""(
    ...1....
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
    ...111..
  )""";
  EXPECT_EQ(0x800600904004038ULL, bitmanip::FromLayout(layout_str));
}

TEST(BitManip, MaskRow) {
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    ........
    ........
    ........
    11111111    
    ........
    ........
  )"""),
            bitmanip::MaskRow(2));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    11111111    
    ........
    ........
    ........
    ........
    ........
    ........
  )"""),
            bitmanip::siderel::MaskRow<Side::WHITE>(6));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    ........
    ........
    ........
    ........
    11111111    
    ........
  )"""),
            bitmanip::siderel::MaskRow<Side::BLACK>(6));
}

TEST(BitManipTest, MaskColumn) {
  EXPECT_EQ(bitmanip::FromLayout(R"""(
      .....1..
      .....1..
      .....1..
      .....1..
      .....1..
      .....1..
      .....1..
      .....1..
    )"""),
            bitmanip::MaskColumn(5));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
      ......1.
      ......1.
      ......1.
      ......1.
      ......1.
      ......1.
      ......1.
      ......1.
    )"""),
            bitmanip::siderel::MaskColumn<Side::WHITE>(6));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
      .1......
      .1......
      .1......
      .1......
      .1......
      .1......
      .1......
      .1......
    )"""),
            bitmanip::siderel::MaskColumn<Side::BLACK>(6));
}

TEST(BitManipTest, PushNorth) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
    ...111..
    ........
  )"""),
            bitmanip::PushNorth(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
    ...111..
    ........
  )"""),
            bitmanip::siderel::PushNorth<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ...1....
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
  )"""),
            bitmanip::siderel::PushNorth<Side::BLACK>(bitboard));
}

TEST(BitManipTest, PushSouth) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ...1....
    ........
    .....11.
    1..1....
    ..1.....
    ........
    ......1.
  )"""),
            bitmanip::PushSouth(bitboard));
}

TEST(BitManipTest, PushEast) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1...1
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ....1...
    ........
    ......11
    .1..1...
    ...1....
    ........
    .......1
    ....111.
  )"""),
            bitmanip::PushEast(bitboard));
}

TEST(BitManipTest, PushWest) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1...1
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ..1.....
    ........
    ....11..
    ..1...1.
    .1......
    ........
    .....1..
    ..111...
  )"""),
            bitmanip::PushWest(bitboard));
}

TEST(BitManipTest, PushNorthEast) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1...1
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ......11
    .1..1...
    ...1....
    ........
    .......1
    ....111.
    ........
  )"""),
            bitmanip::PushNorthEast(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ......11
    .1..1...
    ...1....
    ........
    .......1
    ....111.
    ........
  )"""),
            bitmanip::siderel::PushNorthEast<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ..1.....
    ........
    ....11..
    ..1...1.
    .1......
    ........
    .....1..
  )"""),
            bitmanip::siderel::PushNorthEast<Side::BLACK>(bitboard));
}

TEST(BitManipTest, PushNorthWest) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ...1....
    ........
    .....11.
    1..1...1
    ..1.....
    ........
    ......1.
    ...111..
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ....11..
    ..1...1.
    .1......
    ........
    .....1..
    ..111...
    ........
  )"""),
            bitmanip::PushNorthWest(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ....11..
    ..1...1.
    .1......
    ........
    .....1..
    ..111...
    ........
  )"""),
            bitmanip::siderel::PushNorthWest<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ....1...
    ........
    ......11
    .1..1...
    ...1....
    ........
    .......1
  )"""),
            bitmanip::siderel::PushNorthWest<Side::BLACK>(bitboard));
}

TEST(BitManipTest, FrontFill) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11....
    ........
    ........
    1..1....
    ........
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
  )"""),
            bitmanip::FrontFill(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
  )"""),
            bitmanip::siderel::FrontFill<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
    1.11.1..
  )"""),
            bitmanip::siderel::FrontFill<Side::BLACK>(bitboard));
}

TEST(BitManipTest, RearFill) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11....
    ........
    ........
    1..1....
    ........
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
    1.11.1..
  )"""),
            bitmanip::RearFill(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
    1.11.1..
  )"""),
            bitmanip::siderel::RearFill<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
  )"""),
            bitmanip::siderel::RearFill<Side::BLACK>(bitboard));
}

TEST(BitManipTest, FrontSpan) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11....
    ........
    ........
    1..1....
    ........
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
    ........
  )"""),
            bitmanip::FrontSpan(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
    ........
  )"""),
            bitmanip::siderel::FrontSpan<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
  )"""),
            bitmanip::siderel::FrontSpan<Side::BLACK>(bitboard));
}

TEST(BitManipTest, RearSpan) {
  const U64 bitboard = bitmanip::FromLayout(R"""(
    ........
    ........
    .....1..
    ..11....
    ........
    ........
    1..1....
    ........
  )""");
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
  )"""),
            bitmanip::RearSpan(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    ........
    ........
    ........
    .....1..
    ..11.1..
    ..11.1..
    ..11.1..
    1.11.1..
  )"""),
            bitmanip::siderel::RearSpan<Side::WHITE>(bitboard));
  EXPECT_EQ(bitmanip::FromLayout(R"""(
    1.11.1..
    1.11.1..
    1.11....
    1..1....
    1..1....
    1..1....
    ........
    ........
  )"""),
            bitmanip::siderel::RearSpan<Side::BLACK>(bitboard));
}