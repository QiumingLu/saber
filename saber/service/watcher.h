// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVICE_WATCHER_H_
#define SABER_SERVICE_WATCHER_H_

#include "saber/proto/saber.pb.h"

namespace saber {

class Watcher {
 public:
  Watcher() {}
  virtual ~Watcher() {}

  virtual void Process(const WatchedEvent& event) = 0;
};

}  // namespace saber

#endif  // SABER_SERVICE_WATCHER_H_