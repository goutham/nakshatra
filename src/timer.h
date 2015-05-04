#ifndef TIMER_H
#define TIMER_H

#include "common.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <stdexcept>
#include <time.h>

class Timer {
 public:
  Timer() : timer_expired_(false) {
    struct sigevent sigx;
    sigx.sigev_notify = SIGEV_THREAD;
    sigx.sigev_value.sival_ptr = this;
    sigx.sigev_notify_function = &Timer::Handler;
    sigx.sigev_notify_attributes = NULL;
    if (timer_create(CLOCK_REALTIME, &sigx, &timer_id_) == -1) {
      throw std::runtime_error("Error: timer_create");
    }
  }

  ~Timer() {
    if (!timer_expired_) {
      Stop();
      if (timer_delete(timer_id_) == -1) {
        throw std::runtime_error("Error: timer_delete");
      }
    }
  }

  void Reset() {
    RemoveTimer();
    timer_expired_ = false;
  }

  void Run(long centis) {
    if (timer_expired_) {
      return;
    }
    if (centis == 0) {
      timer_expired_ = true;
      return;
    }
    struct itimerspec val;
    val.it_value.tv_sec = centis / 100;
    val.it_value.tv_nsec = (centis % 100) * 10000000;
    val.it_interval.tv_sec = 0;
    val.it_interval.tv_nsec = 0;
    if (timer_settime(timer_id_, 0, &val, 0) == -1) {
      throw std::runtime_error("Error: timer_settime");
    }
  }

  // Returns time remaining for expiry, in centis.
  double Remaining() const {
    struct itimerspec val;
    if (timer_gettime(timer_id_, &val) == -1) {
      throw std::runtime_error("Error: timer_gettime");
    }
    return val.it_value.tv_sec * 100.0 + val.it_value.tv_nsec / 10000000.0;
  }

  void Stop() {
    if (timer_expired_) {
      return;
    }
    RemoveTimer();
    timer_expired_ = true;
  }

  bool timer_expired() const {
    return timer_expired_;
  }
 private:

  static void Handler(sigval_t  sival) {
    reinterpret_cast<Timer*>(sival.sival_ptr)->Stop();
  }

  void RemoveTimer() {
    struct itimerspec val;
    val.it_value.tv_sec = 0;
    val.it_value.tv_nsec = 0;
    val.it_interval.tv_sec = 0;
    val.it_interval.tv_nsec = 0;
    if (timer_settime(timer_id_, 0, &val, 0) == -1) {
      throw std::runtime_error("Error: timer_settime");
    }
  }

  timer_t timer_id_;
  bool timer_expired_;
};

#endif
