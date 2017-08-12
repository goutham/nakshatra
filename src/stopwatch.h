#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <cstdio>
#include <sys/time.h>

class StopWatch {
public:
  void Start() {
    is_running_ = true;
    gettimeofday(&t1_, NULL);
  }

  void Stop() {
    is_running_ = false;
    gettimeofday(&t2_, NULL);
  }

  // Returns elapsed time in centi seconds.
  double ElapsedTime() {
    if (is_running_) {
      gettimeofday(&t2_, NULL);
    }
    return static_cast<double>((t2_.tv_usec - t1_.tv_usec) +
                               (t2_.tv_sec - t1_.tv_sec) * 1000000) /
           10000;
  }

private:
  struct timeval t1_;
  struct timeval t2_;
  bool is_running_;
};

#endif
