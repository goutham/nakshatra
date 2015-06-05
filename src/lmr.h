#ifndef LMR_H
#define LMR_H

class LMR {
 public:
  LMR(const int full_depth_moves,
      const int reduction_limit,
      const int depth_reduction_factor)
      : full_depth_moves_(full_depth_moves),
        reduction_limit_(reduction_limit),
        depth_reduction_factor_(depth_reduction_factor) {}

  // Returns true if late move reduction should be triggered.
  bool CanReduce(const int move_index, const int search_depth_remaining) const {
    return move_index >= full_depth_moves_ &&
           search_depth_remaining >= reduction_limit_;
  }

  int DepthReductionFactor() const {
    return depth_reduction_factor_;
  }

 private:
  const int full_depth_moves_;
  const int reduction_limit_;
  const int depth_reduction_factor_;
};

#endif
