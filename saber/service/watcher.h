// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVICE_WATCHER_H_
#define SABER_SERVICE_WATCHER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "saber/proto/saber.pb.h"

namespace saber {

class Watcher {
 public:
  Watcher() {}
  virtual ~Watcher() {}

  virtual void Process(const WatchedEvent& event) = 0;

 private:
  // No copying allowed
  Watcher(const Watcher&);
  void operator=(const Watcher&);
};

typedef std::unordered_set<Watcher*> WatcherSet;
typedef std::unordered_set<std::string> PathSet;
typedef std::unique_ptr<WatcherSet> WatcherSetPtr;
typedef std::unique_ptr<PathSet> PathSetPtr;

}  // namespace saber

#endif  // SABER_SERVICE_WATCHER_H_
