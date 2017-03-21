// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_WATCH_MANAGER_H_
#define SABER_SERVER_SERVER_WATCH_MANAGER_H_

#include <memory>
#include <string>
#include <set>
#include <unordered_map>

#include "saber/service/watcher.h"

namespace saber {

typedef std::unique_ptr<std::set<Watcher*> > WatcherSetPtr;
typedef std::unique_ptr<std::set<std::string> > PathSetPtr;

class ServerWatchManager {
 public:
  ServerWatchManager();
  ~ServerWatchManager();

  void AddWatch(const std::string& path, Watcher* watcher);
  void RemoveWatch(Watcher* watcher);

  WatcherSetPtr TriggerWatcher(
      const std::string& path, const WatchedEvent& event);
  WatcherSetPtr TriggerWatcher(
      const std::string& path, const WatchedEvent& event, WatcherSetPtr p);

 private:
  std::unordered_map<std::string, WatcherSetPtr> path_to_watches_;
  std::unordered_map<Watcher*, PathSetPtr> watch_to_paths_;

  // No copying allowed
  ServerWatchManager(const ServerWatchManager&);
  void operator=(const ServerWatchManager&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_WATCH_MANAGER_H_
