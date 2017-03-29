// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/client_watch_manager.h"

namespace saber {

ClientWatchManager::ClientWatchManager() {
}

ClientWatchManager::~ClientWatchManager() {
}

void ClientWatchManager::AddDataWatch(
    const std::string& path, Watcher* watcher) {
  auto it = data_watches_.find(path);
  if (it == data_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    data_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddExistWatch(
    const std::string& path, Watcher* watcher) {
  auto it = exist_watches_.find(path);
  if (it == exist_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    exist_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddChildWatch(
    const std::string& path, Watcher* watcher) {
  auto it = child_watches_.find(path);
  if (it == child_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    child_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::RemoveDataWatch(
    const std::string& path, Watcher* watcher) {
  auto it = data_watches_.find(path);
  if (it != data_watches_.end()) {
    it->second->erase(watcher);
  }
}

void ClientWatchManager::RemoveExistWatch(
    const std::string& path, Watcher* watcher) {
  auto it = exist_watches_.find(path);
  if (it != exist_watches_.end()) {
    it->second->erase(watcher);
  }
}

void ClientWatchManager::RemoveChildWatch(
    const std::string& path, Watcher* watcher) {
  auto it  = child_watches_.find(path);
  if (it != child_watches_.end()) {
    it->second->erase(watcher);
  }
}

}  // namespace saber
