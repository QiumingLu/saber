// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_WATCH_MANAGER_H_
#define SABER_SERVER_SERVER_WATCH_MANAGER_H_

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "saber/service/watcher.h"

namespace saber {

class ServerWatchManager {
 public:
  ServerWatchManager();
  ~ServerWatchManager();

  void AddWatcher(const std::string& path, Watcher* watcher);
  void RemoveWatcher(Watcher* watcher);

  void TriggerWatcher(const std::string& path, EventType type);

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, std::unordered_set<Watcher*>> path_to_watches_;
  std::unordered_map<Watcher*, std::unordered_set<std::string>> watch_to_paths_;

  // No copying allowed
  ServerWatchManager(const ServerWatchManager&);
  void operator=(const ServerWatchManager&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_WATCH_MANAGER_H_
