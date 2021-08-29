// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_RUNLOOP_THREAD_H_
#define SABER_UTIL_RUNLOOP_THREAD_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "saber/util/runloop.h"

namespace saber {

class RunLoopThread {
 public:
  RunLoopThread();
  ~RunLoopThread();

  RunLoop* Loop();

 private:
  void ThreadFunc();

  RunLoop* loop_;
  std::mutex mu_;
  std::condition_variable cond_;
  std::unique_ptr<std::thread> thread_;

  // No copying allowed
  RunLoopThread(const RunLoopThread&);
  void operator=(const RunLoopThread&);
};

}  // namespace saber

#endif  // SABER_UTIL_RUNLOOP_THREAD_H_
