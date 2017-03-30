// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_MUTEXLOCK_H_
#define SABER_UTIL_MUTEXLOCK_H_

#include "saber/util/mutex.h"

namespace saber {

class MutexLock {
 public:
  explicit MutexLock(Mutex* mutex)
      : mutex_(mutex) {
    mutex_->Lock();
  }

  ~MutexLock() { mutex_->UnLock(); }

 private:
  Mutex* const mutex_;

  // No copying allowed
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};

}  // namespace saber

#endif  // SABER_UTIL_MUTEXLOCK_H_
