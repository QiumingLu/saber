// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_watch_manager.h"

namespace saber {

ServerWatchManager::ServerWatchManager() {
}

ServerWatchManager::~ServerWatchManager() {
}

void ServerWatchManager::AddWatch(
    const std::string& path, Watcher* watcher) {
  auto i  = watch_table_.find(path);
  if (i != watch_table_.end()) {
    i->second->insert(watcher);
  } else {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    watch_table_.insert(std::make_pair(path, std::move(watches)));
  }

  auto j = path_table_.find(watcher);
  if (j != path_table_.end()) {
    j->second->insert(path);
  } else {
    PathSetPtr paths(new std::set<std::string>());
    paths->insert(path);
    path_table_.insert(std::make_pair(watcher, std::move(paths)));
  }
}

void ServerWatchManager::RemoveWatch(Watcher* watcher) {
  auto i = path_table_.find(watcher);
  if (i != path_table_.end()) {
    for (auto& j : *(i->second)) {
      auto k =  watch_table_.find(j);
      k->second->erase(watcher);
      if (k->second->empty()) {
        watch_table_.erase(j);
      }
    }
    path_table_.erase(i);
  }
}

WatcherSetPtr ServerWatchManager::TriggerWatcher(const std::string& path) {
  WatcherSetPtr watches;
  auto i = watch_table_.find(path);
  if (i != watch_table_.end()) {
    for (auto& j : *(i->second)) {
      auto k = path_table_.find(j);
      if (k != path_table_.end()) {
        k->second->erase(path);
      }
    }
    watches.swap(i->second);
    watch_table_.erase(i);
  }
  return watches;
}

}  // namespace saber
