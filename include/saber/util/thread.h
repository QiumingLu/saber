// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_THREAD_H_
#define SABER_UTIL_THREAD_H_

#include <stdint.h>
#include <pthread.h>

namespace saber {

namespace CurrentThread {

extern __thread uint64_t  cached_tid;

extern uint64_t Tid();

}  // namespace CurrentThread

class Thread {
 public:
  Thread();
  ~Thread();

  void Start(void* (*function)(void*), void* arg);
  void Join();

  bool Started() const { return started_; }
  pthread_t gettid() const { return thread_; }

 private:
  void PthreadCall(const char* label, int result);

  bool started_;
  bool joined_;
  pthread_t thread_;

  // No copying allowed
  Thread(const Thread&);
  void operator=(const Thread&);
};

}  // namespace saber

#endif  // SABER_UTIL_THREAD_H_
