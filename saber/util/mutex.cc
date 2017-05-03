// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/util/mutex.h"

#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "saber/util/logging.h"

namespace saber {

static void PthreadCall(const char* label, int result) {
  if (result != 0) {
    LOG_FATAL("%s: %s", label, strerror(result));
  }
}

Mutex::Mutex() {
  PthreadCall("pthread_mutex_init", pthread_mutex_init(&mutex_, nullptr));
}

Mutex::~Mutex() {
  PthreadCall("pthread_mutex_destory", pthread_mutex_destroy(&mutex_));
}

void Mutex::Lock() {
  PthreadCall("pthread_mutex_lock", pthread_mutex_lock(&mutex_));
}

void Mutex::UnLock() {
  PthreadCall("pthread_mutex_unlock", pthread_mutex_unlock(&mutex_));
}

Condition::Condition(Mutex* mutex) : mutex_(mutex) {
  PthreadCall("pthread_cond_init", pthread_cond_init(&cond_, nullptr));
}

Condition::~Condition() {
  PthreadCall("pthread_cond_destory", pthread_cond_destroy(&cond_));
}

void Condition::Wait() {
  PthreadCall("pthread_cond_wait", pthread_cond_wait(&cond_, &mutex_->mutex_));
}

bool Condition::Wait(uint64_t micros_second) {
  struct timeval now;
  struct timespec outtime;
  gettimeofday(&now, nullptr);
  outtime.tv_sec =
    now.tv_sec + static_cast<time_t>(micros_second / (1000 * 1000));
  outtime.tv_nsec =
      now.tv_usec * 1000 +
      static_cast<suseconds_t>((micros_second % (1000 * 1000)) * 1000);
  outtime.tv_sec += outtime.tv_nsec / (1000 * 1000 * 1000);
  outtime.tv_nsec %= (1000 * 1000 * 1000);
  int res = pthread_cond_timedwait(&cond_, &mutex_->mutex_, &outtime);
  if (res != 0 && res != ETIMEDOUT) {
    PthreadCall("pthread_cond_timedwait", res);
  }
  return res == 0;
}

void Condition::Signal() {
  PthreadCall("pthread_cond_signal", pthread_cond_signal(&cond_));
}

void Condition::SignalAll() {
  PthreadCall("pthread_cond_broadcast", pthread_cond_broadcast(&cond_));
}

void InitOnce(OnceType* once, void (*initializer)()) {
  PthreadCall("pthread_once", pthread_once(once, initializer));
}

}  // namespace saber
