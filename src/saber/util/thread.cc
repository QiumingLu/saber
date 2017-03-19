// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/util/thread.h"

#include <assert.h>
#include <string.h>

#include "saber/util/logging.h"

namespace saber {

Thread::Thread()
     : started_(false),
       joined_(false),
       thread_(0) {
}

Thread::~Thread() {
  if (started_ && !joined_) {
    pthread_detach(thread_);
  }
}

void Thread::Start(void* (*function)(void* arg), void* arg) {
  assert(!started_);
  started_ = true;
  PthreadCall("pthread_create",
              pthread_create(&thread_, nullptr, function, arg));
}

void Thread::Join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  PthreadCall("pthread_join", pthread_join(thread_, nullptr));
}

void Thread::PthreadCall(const char* label, int result) {
  if (result != 0) {
    LOG_FATAL("%s: %s\n", label, strerror(result));
  }
}

}  // namespace saber
