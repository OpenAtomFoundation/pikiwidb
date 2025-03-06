//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//

#ifndef STAGE_TIMER_H_
#define STAGE_TIMER_H_

#include "pstd/include/env.h"

namespace pstd {
class StageTimer {
 public:
  explicit StageTimer (uint64_t* metric_ms, bool enabled)
      : perf_counter_enabled_(enabled),
        start_(0),
        metric_ms_(metric_ms) {}

  ~StageTimer() { Stop(); }

  void Start() {
    if (perf_counter_enabled_) {
      start_ = time_now();
    }
  }

  void Measure() {
    if (start_) {
      uint64_t now = time_now();
      *metric_ms_ += (now - start_) / 1000;
      start_ = now;
    }
  }

  void Stop() {
    if (start_) {
      uint64_t duration = (time_now() - start_) / 1000;
      if (perf_counter_enabled_) {
        *metric_ms_ += duration;
      }
      start_ = 0;
    }
  }

 private:
  uint64_t time_now() {
    return NowMicros();
  }

  const bool perf_counter_enabled_;
  uint64_t start_;
  uint64_t* metric_ms_;
};
}  // namespace pstd
#endif
