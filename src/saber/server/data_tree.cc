// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_tree.h"

namespace saber {

DataTree::DataTree() {
  nodes_.insert(
      std::make_pair("", std::unique_ptr<DataNode>(new DataNode())));
}

DataTree::~DataTree() {
}

int DataTree::CreateNode(
    const std::string& path, const std::string& data) {
  int result = 0;
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);
  auto it = nodes_.find(parent);
  if (it != nodes_.end()) {
    bool res = it->second->AddChild(child);
    if (res) {
      DataNode* node = new DataNode(data);
      nodes_.insert(std::make_pair(path, std::unique_ptr<DataNode>(node)));
      WatchedEvent event;
      data_watches_.TriggerWatcher(path, event);
      child_watches_.TriggerWatcher(parent.empty() ? "/" : parent, event);
    } else {
      result = -2;
    }
  } else {
    result = -1;
  }
  return result;
}

int DataTree::DeleteNode(const std::string& path) {
  int result = 0;
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);
  auto it =  nodes_.find(path);
  if (it != nodes_.end()) {
    nodes_.erase(it);
    it = nodes_.find(parent);
    if (it != nodes_.end()) {
      it->second->RemoveChild(child);
      WatchedEvent event;
      WatcherSetPtr p = data_watches_.TriggerWatcher(path, event);
      child_watches_.TriggerWatcher(path, event, std::move(p));
      child_watches_.TriggerWatcher(parent.empty() ? "/" : parent, event);
    } else {
      result = -1;
    }
  } else {
    result = -2;
  }
  return result;
}

bool DataTree::SetData(const std::string& path, const std::string& data) {
  auto it  =nodes_.find(path);
  if (it != nodes_.end()) {
    WatchedEvent event;
    data_watches_.TriggerWatcher(path, event);
    return true;
  }
  return false;
}

bool DataTree::GetData(const std::string& path,
                       Watcher* watcher, std::string* data) {
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    *data = it->second->GetData();
    if (watcher != nullptr) {
      data_watches_.AddWatch(path, watcher);
    }
    return true;
  }
  return false;
}

}  // namespace saber
