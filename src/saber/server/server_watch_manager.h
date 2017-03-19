// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_WATCH_MANAGER_H_
#define SABER_SERVER_SERVER_WATCH_MANAGER_H_

#include <set>
#include <unordered_map>

#include "saber/service/watcher.h"

namespace saber {

class ServerWatchManager {
 public:
  ServerWatchManager();
  ~ServerWatchManager();

  void AddWatch(const std::string& path, Watcher* watcher);
  void RemoveWatch(Watcher* watcher);

  void TriggerWatcher(const std::string& path, std::set<Watcher*>* result);

 private:
  std::unordered_map<std::string, std::set<Watcher*> > watches_;

  // No copying allowed
  ServerWatchManager(const ServerWatchManager&);
  void operator=(const ServerWatchManager&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_WATCH_MANAGER_H_
