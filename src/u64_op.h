#ifndef U64_OP_H
#define U64_OP_H

#include "common.h"

class U64Op {
 public:
  U64Op(const U64& value)
      : value_(value) {}

  // Returns the index of the next right most bit in 'value_'. Returns -1 if
  // 'value_' == 0ULL. This operation is non-idempotent.
  int NextRightMostBitIndex() {
    if (!value_) return -1;
    const U64 value_with_right_most_bit_set = (value_ & -value_);
    const int right_most_bit_index = log2U(value_with_right_most_bit_set);
    value_ ^= value_with_right_most_bit_set;
    return right_most_bit_index;
  }

 private:
  U64 value_;
};

#endif
