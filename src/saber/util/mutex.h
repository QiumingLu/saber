// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_MUTEX_H_
#define SABER_UTIL_MUTEX_H_

#include <pthread.h>
#include <stdint.h>

namespace saber {

class Condition;

class Mutex {
 public:
  Mutex();
  ~Mutex();

  void Lock();
  void UnLock();
  void AssertHeld() { }

 private:
  friend class Condition;
  pthread_mutex_t mutex_;

  // No copying allowed
  Mutex(const Mutex&);
  void operator=(const Mutex&);
};

class Condition {
 public:
  explicit Condition(Mutex* mutex);
  ~Condition();

  void Wait();
  bool Wait(uint64_t micros_second);
  void Signal();
  void SignalAll();

 private:
  pthread_cond_t cond_;
  Mutex* mutex_;

  // No copying allowed
  Condition(const Condition&);
  void operator=(const Condition&);
};

}  // namespace saber

#endif  // SABER_UTIL_MUTEX_H_
