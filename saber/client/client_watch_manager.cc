// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/client_watch_manager.h"

#include <utility>

#include "saber/util/logging.h"

namespace saber {

ClientWatchManager::ClientWatchManager(Watcher* watcher) : watcher_(watcher) {}

ClientWatchManager::~ClientWatchManager() {}

void ClientWatchManager::AddDataWatch(const std::string& path,
                                      Watcher* watcher) {
  auto it = data_watches_.find(path);
  if (it == data_watches_.end()) {
    WatcherSetPtr watches(new WatcherSet());
    watches->insert(watcher);
    data_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddExistsWatch(const std::string& path,
                                        Watcher* watcher) {
  auto it = exists_watches_.find(path);
  if (it == exists_watches_.end()) {
    WatcherSetPtr watches(new WatcherSet());
    watches->insert(watcher);
    exists_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddChildWatch(const std::string& path,
                                       Watcher* watcher) {
  auto it = child_watches_.find(path);
  if (it == child_watches_.end()) {
    WatcherSetPtr watches(new WatcherSet());
    watches->insert(watcher);
    child_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

WatcherSetPtr ClientWatchManager::Trigger(const WatchedEvent& event) {
  WatcherSetPtr result;
  switch (event.type()) {
    case ET_NONE: {
      result.reset(new WatcherSet());
      if (watcher_) {
        result->insert(watcher_);
      }
      for (auto& i : data_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      for (auto& i : exists_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      for (auto& i : child_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      // FIXME Maybe auto reset watch will be better when state is connected?
      data_watches_.clear();
      exists_watches_.clear();
      child_watches_.clear();
      break;
    }
    case ET_NODE_CREATED:
    case ET_NODE_DATA_CHANGED: {
      auto i = data_watches_.find(event.path());
      if (i != data_watches_.end()) {
        result.swap(i->second);
        data_watches_.erase(i);
      }
      auto j = exists_watches_.find(event.path());
      if (j != exists_watches_.end()) {
        if (result) {
          result->insert(j->second->begin(), j->second->end());
        } else {
          result.swap(j->second);
        }
        exists_watches_.erase(j);
      }
      break;
    }
    case ET_NODE_DELETED: {
      auto i = data_watches_.find(event.path());
      if (i != data_watches_.end()) {
        result.swap(i->second);
        data_watches_.erase(i);
      }
      auto j = child_watches_.find(event.path());
      if (j != child_watches_.end()) {
        if (result) {
          result->insert(j->second->begin(), j->second->end());
        } else {
          result.swap(j->second);
        }
        child_watches_.erase(j);
      }
      break;
    }
    case ET_NODE_CHILDREN_CHANGED: {
      auto i = child_watches_.find(event.path());
      if (i != child_watches_.end()) {
        result.swap(i->second);
        child_watches_.erase(i);
      }
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid watched event type.");
      break;
    }
  }
  return result;
}

}  // namespace saber
