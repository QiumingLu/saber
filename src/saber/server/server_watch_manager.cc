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
  auto i  = path_to_watches_.find(path);
  if (i != path_to_watches_.end()) {
    i->second->insert(watcher);
  } else {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    path_to_watches_.insert(std::make_pair(path, std::move(watches)));
  }

  auto j = watch_to_paths_.find(watcher);
  if (j != watch_to_paths_.end()) {
    j->second->insert(path);
  } else {
    PathSetPtr paths(new std::set<std::string>());
    paths->insert(path);
    watch_to_paths_.insert(std::make_pair(watcher, std::move(paths)));
  }
}

void ServerWatchManager::RemoveWatch(Watcher* watcher) {
  auto i = watch_to_paths_.find(watcher);
  if (i != watch_to_paths_.end()) {
    for (auto& j : *(i->second)) {
      auto k =  path_to_watches_.find(j);
      k->second->erase(watcher);
      if (k->second->empty()) {
        path_to_watches_.erase(j);
      }
    }
    watch_to_paths_.erase(i);
  }
}

WatcherSetPtr ServerWatchManager::TriggerWatcher(
    const std::string& path, const WatchedEvent& event) {
  WatcherSetPtr watches;
  auto i = path_to_watches_.find(path);
  if (i != path_to_watches_.end()) {
    for (auto& j : *(i->second)) {
      auto k = watch_to_paths_.find(j);
      k->second->erase(path);
      if (k->second->empty()) {
        watch_to_paths_.erase(k);
      }
      j->Process(event);
    }
    watches.swap(i->second);
    path_to_watches_.erase(i);
  }
  return watches;
}

}  // namespace saber
