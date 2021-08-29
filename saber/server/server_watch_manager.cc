// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_watch_manager.h"

#include <utility>

namespace saber {

ServerWatchManager::ServerWatchManager() {}

ServerWatchManager::~ServerWatchManager() {}

void ServerWatchManager::AddWatcher(const std::string& path, Watcher* watcher) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto i = path_to_watches_.find(path);
  if (i != path_to_watches_.end()) {
    i->second->insert(watcher);
  } else {
    WatcherSetPtr watches(new WatcherSet());
    watches->insert(watcher);
    path_to_watches_.insert(std::make_pair(path, std::move(watches)));
  }

  auto j = watch_to_paths_.find(watcher);
  if (j != watch_to_paths_.end()) {
    j->second->insert(path);
  } else {
    PathSetPtr paths(new PathSet());
    paths->insert(path);
    watch_to_paths_.insert(std::make_pair(watcher, std::move(paths)));
  }
}

void ServerWatchManager::RemoveWatcher(Watcher* watcher) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto i = watch_to_paths_.find(watcher);
  if (i != watch_to_paths_.end()) {
    for (auto& j : *(i->second)) {
      auto k = path_to_watches_.find(j);
      k->second->erase(watcher);
      if (k->second->empty()) {
        path_to_watches_.erase(j);
      }
    }
    watch_to_paths_.erase(i);
  }
}

WatcherSetPtr ServerWatchManager::TriggerWatcher(const std::string& path,
                                                 EventType type) {
  return TriggerWatcher(path, type, nullptr);
}

WatcherSetPtr ServerWatchManager::TriggerWatcher(const std::string& path,
                                                 EventType type,
                                                 WatcherSetPtr p) {
  WatcherSetPtr watches;
  std::unique_lock<std::mutex> lock(mutex_);
  auto i = path_to_watches_.find(path);
  if (i != path_to_watches_.end()) {
    WatchedEvent event;
    event.set_state(SS_CONNECTED);
    event.set_type(type);
    event.set_path(path);

    for (auto& j : *(i->second)) {
      auto k = watch_to_paths_.find(j);
      k->second->erase(path);
      if (k->second->empty()) {
        watch_to_paths_.erase(k);
      }
      if (p && p->find(j) != p->end()) {
        continue;
      }
      j->Process(event);
    }
    watches.swap(i->second);
    path_to_watches_.erase(i);
  }
  return watches;
}

}  // namespace saber
