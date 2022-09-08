// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_TIMERLIST_H_
#define SABER_UTIL_TIMERLIST_H_

#include <stdint.h>
#include <functional>
#include <set>
#include <utility>

namespace saber {

class RunLoop;
class Timer;
typedef std::pair<uint64_t, Timer*> TimerId;
typedef std::function<void ()> TimerProcCallback;

class TimerList {
 public:
  explicit TimerList(RunLoop* loop);
  ~TimerList();

  TimerId RunAt(uint64_t ms_value, const TimerProcCallback& cb);
  TimerId RunAt(uint64_t ms_value, TimerProcCallback&& cb);

  TimerId RunAfter(uint64_t ms_delay, const TimerProcCallback& cb);
  TimerId RunAfter(uint64_t ms_delay, TimerProcCallback&& cb);

  TimerId RunEvery(uint64_t ms_interval, const TimerProcCallback& cb);
  TimerId RunEvery(uint64_t ms_interval, TimerProcCallback&& cb);

  void Remove(TimerId timer);

  uint64_t TimeoutMs() const;
  void RunTimerProcs();

 private:
  void InsertInLoop(TimerId timer);

  uint64_t last_time_out_;

  RunLoop* loop_;
  std::set<Timer*> timer_ptrs_;
  std::set<TimerId> timers_;

  // No copying allowed
  TimerList(const TimerList&);
  void operator=(const TimerList&);
};

}  // namespace saber

#endif  // SABER_UTIL_TIMERLIST_H_
