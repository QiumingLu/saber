// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/client_watch_manager.h"

#include <set>
#include <utility>

#include "saber/util/logging.h"

namespace saber {

ClientWatchManager::ClientWatchManager(bool auto_watch_reset)
    : auto_watch_reset_(auto_watch_reset) {}

ClientWatchManager::~ClientWatchManager() {}

void ClientWatchManager::AddDataWatch(const std::string& path,
                                      Watcher* watcher) {
  auto it = data_watches_.find(path);
  if (it == data_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    data_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddExistWatch(const std::string& path,
                                       Watcher* watcher) {
  auto it = exist_watches_.find(path);
  if (it == exist_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
    watches->insert(watcher);
    exist_watches_.insert(std::make_pair(path, std::move(watches)));
  } else {
    it->second->insert(watcher);
  }
}

void ClientWatchManager::AddChildWatch(const std::string& path,
                                       Watcher* watcher) {
  auto it = child_watches_.find(path);
  if (it == child_watches_.end()) {
    WatcherSetPtr watches(new std::set<Watcher*>());
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
      result.reset(new std::set<Watcher*>());
      for (auto& i : data_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      for (auto& i : exist_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      for (auto& i : child_watches_) {
        result->insert(i.second->begin(), i.second->end());
      }
      if (event.state() != SS_CONNECTED && !auto_watch_reset_) {
        data_watches_.clear();
        exist_watches_.clear();
        child_watches_.clear();
      }
      break;
    }
    case ET_NODE_CREATED:
    case ET_NODE_DATA_CHANGED: {
      auto i = data_watches_.find(event.path());
      if (i != data_watches_.end()) {
        result.swap(i->second);
        data_watches_.erase(i);
      }
      auto j = exist_watches_.find(event.path());
      if (j != exist_watches_.end()) {
        if (result) {
          result->insert(j->second->begin(), j->second->end());
        } else {
          result.swap(j->second);
        }
        exist_watches_.erase(j);
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
    case ET_DATA_WATCH_REMOVED: {
      break;
    }
    case ET_CHILD_WATCH_REMOVED: {
      break;
    }
    default: {
      LOG_ERROR("Invalid watched event type.");
      break;
    }
  }
  return result;
}

}  // namespace saber
