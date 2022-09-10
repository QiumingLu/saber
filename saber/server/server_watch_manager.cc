// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_watch_manager.h"

namespace saber {

ServerWatchManager::ServerWatchManager() {}

ServerWatchManager::~ServerWatchManager() {}

void ServerWatchManager::AddWatcher(const std::string& path, Watcher* watcher) {
  std::lock_guard<std::mutex> lock(mutex_);
  path_to_watches_[path].insert(watcher);
  watch_to_paths_[watcher].insert(path);
}

void ServerWatchManager::RemoveWatcher(Watcher* watcher) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto i = watch_to_paths_.find(watcher);
  if (i != watch_to_paths_.end()) {
    for (const auto& path : i->second) {
      auto j = path_to_watches_.find(path);
      j->second.erase(watcher);
      if (j->second.empty()) {
        path_to_watches_.erase(j);
      }
    }
    watch_to_paths_.erase(i);
  }
}

void ServerWatchManager::TriggerWatcher(const std::string& path, EventType type) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto i = path_to_watches_.find(path);
  if (i != path_to_watches_.end()) {
    WatchedEvent event;
    event.set_state(SS_CONNECTED);
    event.set_type(type);
    event.set_path(path);

    for (auto watcher : i->second) {
      auto j = watch_to_paths_.find(watcher);
      j->second.erase(path);
      if (j->second.empty()) {
        watch_to_paths_.erase(j);
      }
      watcher->Process(event);
    }
    path_to_watches_.erase(i);
  }
}

}  // namespace saber
