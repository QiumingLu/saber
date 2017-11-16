// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_COUNTDOWNLATCH_H_
#define SABER_UTIL_COUNTDOWNLATCH_H_

#include "saber/util/mutexlock.h"

namespace saber {

class CountDownLatch {
 public:
  explicit CountDownLatch(int count)
      : mutex_(), cond_(&mutex_), count_(count) {}

  void Wait() {
    MutexLock lock(&mutex_);
    while (count_ > 0) {
      cond_.Wait();
    }
  }

  void CountDown() {
    MutexLock lock(&mutex_);
    --count_;
    if (count_ == 0) {
      cond_.Signal();
    }
  }

  int GetCount() const {
    MutexLock lock(&mutex_);
    return count_;
  }

 private:
  mutable Mutex mutex_;
  Condition cond_;
  int count_;

  // No copying allowed
  CountDownLatch(const CountDownLatch&);
  void operator=(const CountDownLatch&);
};

}  // namespace saber

#endif  // SABER_UTIL_COUNTDOWNLATCH_H_