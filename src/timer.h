#ifndef TIMER_H
#define TIMER_H

#include <atomic>
#include <chrono>
#include <cstdint>

class Timer {
public:
  void Run(int32_t centis) {
    centis_.store(centis, std::memory_order_relaxed);
    start_ = std::chrono::steady_clock::now();
  }

  void Invalidate() { centis_.store(-1, std::memory_order_relaxed); }

  bool Lapsed() const {
    auto now = std::chrono::steady_clock::now();
    auto centis = centis_.load(std::memory_order_relaxed);
    return (centis >= 0 &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start_)
                        .count() /
                    10 >
                centis) ||
           (centis < 0);
  }

private:
  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::atomic<int32_t> centis_;
};

#endif
