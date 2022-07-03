#ifndef TIMER_H
#define TIMER_H

#include <atomic>
#include <chrono>
#include <cstdint>

class Timer {
public:
  void Run(int32_t centis) {
    centis_ = centis;
    start_ = std::chrono::steady_clock::now();
  }
  void Invalidate() { centis_ = -1; }
  bool Lapsed() const {
    auto now = std::chrono::steady_clock::now();
    return (centis_ >= 0 &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start_)
                        .count() /
                    10 >
                centis_) ||
           (centis_ < 0);
  }

private:
  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::atomic<int32_t> centis_;
};

#endif
