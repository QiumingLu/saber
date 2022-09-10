// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/client_watch_manager.h"

#include "saber/util/logging.h"

namespace saber {

ClientWatchManager::ClientWatchManager(Watcher* watcher) : watcher_(watcher) {}

ClientWatchManager::~ClientWatchManager() {}

void ClientWatchManager::AddDataWatcher(const std::string& path,
                                      Watcher* watcher) {
  data_watches_[path].insert(watcher);
}

void ClientWatchManager::AddChildWatcher(const std::string& path,
                                       Watcher* watcher) {
  child_watches_[path].insert(watcher);
}

void ClientWatchManager::TriggerWatcher(const WatchedEvent& event) {
  std::unordered_set<Watcher*> watchers;
  switch (event.type()) {
    case ET_NONE: {
      if (watcher_) {
        watchers.insert(watcher_);
      }
      for (auto& i : data_watches_) {
        watchers.insert(i.second.begin(), i.second.end());
      }
      for (auto& i : child_watches_) {
        watchers.insert(i.second.begin(), i.second.end());
      }
      // FIXME Maybe auto reset watch will be better when state is connected?
      data_watches_.clear();
      child_watches_.clear();
      break;
    }
    case ET_NODE_CREATED:
    case ET_NODE_DELETED:
    case ET_NODE_DATA_CHANGED: {
      auto it = data_watches_.find(event.path());
      if (it != data_watches_.end()) {
        watchers.swap(it->second);
        data_watches_.erase(it);
      }
      break;
    }
    case ET_NODE_CHILDREN_CHANGED: {
      auto it = child_watches_.find(event.path());
      if (it != child_watches_.end()) {
        watchers.swap(it->second);
        child_watches_.erase(it);
      }
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid watched event type.");
      break;
    }
  }
  for (auto& it : watchers) {
    it->Process(event);
  }
}

}  // namespace saber