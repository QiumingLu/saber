// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_BLOCKINGQUEUE_H_
#define SABER_UTIL_BLOCKINGQUEUE_H_

#include <assert.h>
#include <queue>

#include "saber/util/mutex.h"
#include "saber/util/mutexlock.h"

namespace saber {

template<typename T>
class BlockingQueue {
 public:
  BlockingQueue() : mutex_(), cond_(&mutex_) { }

  void push(const T& t) {
    MutexLock lock(&mutex_);
    if (queue_.empty()) {
      cond_.Signal();
    }
    queue_.push(t);
  }

  void pop() {
    MutexLock lock(&mutex_);
    while (queue_.empty()) {
      cond_.Wait();
    }
    assert(!queue_.empty());
    queue_.pop();
  }

  T take() {
    MutexLock lock(&mutex_);
    while (queue_.empty()) {
      cond_.Wait();
    }
    assert(!queue_.empty());
    T t(queue_.front());
    queue_.pop();
    return t;
  }

  bool empty() const {
    MutexLock lock(&mutex_);
    return queue_.empty();
  }

  size_t size() const {
    MutexLock lock(&mutex_);
    return queue_.size();
  }

 private:
  mutable Mutex mutex_;
  Condition cond_;
  std::queue<T> queue_;

  // No copying allowed
  BlockingQueue(const BlockingQueue&);
  void operator=(const BlockingQueue&);
};

}  // namespace mythread

#endif  // SABER_UTIL_BLOCKINGQUEUE_H_
