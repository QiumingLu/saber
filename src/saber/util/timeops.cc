// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/util/timeops.h"

#include <sys/time.h>
#include <time.h>

namespace saber {

uint64_t NowMillis() {
  return (NowMicros() / 1000);
}

uint64_t NowMicros() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<uint64_t>(tv.tv_sec)*1000000 + tv.tv_usec;
}

}  // namespace saber
