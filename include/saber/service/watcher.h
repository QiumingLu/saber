// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVICE_WATCHER_H_
#define SABER_SERVICE_WATCHER_H_

#include "saber/service/watched_event.h"

namespace saber {

class Watcher {
 public:
  Watcher();
  ~Watcher();

  virtual void Process(const WatchedEvent& event) = 0;

 private:
  Watcher(const Watcher&);
  void operator=(const Watcher&);
};

}  // namespace saber

#endif  // SABER_SERVICE_WATCHER_H_
