// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_DEFAULT_MAIN_WATCHER_H
#define SABER_DEFAULT_MAIN_WATCHER_H

#include "saber/main/to_string.h"
#include "saber/service/watcher.h"

namespace saber {

class DefaultWatcher : public saber::Watcher {
 public:
  DefaultWatcher() {}

  virtual void Process(const saber::WatchedEvent& event) {
    printf("\n------------------------------------------------------------\n");
    printf("%s", ToString(event).c_str());
    printf("\n------------------------------------------------------------\n");
  }
};

}  // namespace saber

#endif  // SABER_DEFAULT_MAIN_WATCHER_H
