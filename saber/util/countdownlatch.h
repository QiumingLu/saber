// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_COUNTDOWNLATCH_H_
#define SABER_UTIL_COUNTDOWNLATCH_H_

#include <condition_variable>
#include <mutex>

namespace saber {

class CountDownLatch {
 public:
  explicit CountDownLatch(int count)
      : count_(count) {}

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (count_ > 0) {
      cond_.wait(lock);
    }
  }

  void CountDown() {
    std::unique_lock<std::mutex> lock(mutex_);
    --count_;
    if (count_ == 0) {
      cond_.notify_one();
    }
  }

  int GetCount() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return count_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable cond_;
  int count_;

  // No copying allowed
  CountDownLatch(const CountDownLatch&);
  void operator=(const CountDownLatch&);
};

}  // namespace saber

#endif  // SABER_UTIL_COUNTDOWNLATCH_H_
