// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_tree.h"
#include "saber/util/mutexlock.h"

namespace saber {

DataTree::DataTree() {
  nodes_.insert(
      std::make_pair("", std::unique_ptr<DataNode>(new DataNode())));
}

DataTree::~DataTree() {
}

void DataTree::CreateNode(const std::string& path, const std::string& data,
                          CreateResponse* response) {
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);
/*
  Stat stat;
  stat.set_create_id();
  stat.set_modified_id();
  stat.set_created_time();
  stat.set_version();
  stat.set_children_version();
  stat.set_acl_version();
  stat.set_ephemeral_id();
  stat.set_data_len();
  stat.set_children_num();
  stat.set_children_id();
*/
  MutexLock lock(&mutex_);
  auto it = nodes_.find(parent);
  if (it != nodes_.end()) {
    bool res = it->second->AddChild(child);
    if (res) {
      DataNode* node = new DataNode(data);
      nodes_.insert(std::make_pair(path, std::unique_ptr<DataNode>(node)));
      data_watches_.TriggerWatcher(path, ET_NODE_CREATED);
      child_watches_.TriggerWatcher(
          parent.empty() ? "/" : parent, ET_NODE_CHILDREN_CHANGED);
    } else {
      response->set_code(RC_NODE_EXISTS);
    }
  } else {
    response->set_code(RC_NO_PARENT);
  }
}

void DataTree::DeleteNode(const std::string& path,
                          DeleteResponse* response) {
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);
  MutexLock lock(&mutex_);
  auto it =  nodes_.find(path);
  if (it != nodes_.end()) {
    nodes_.erase(it);
    it = nodes_.find(parent);
    if (it != nodes_.end()) {
      it->second->RemoveChild(child);
      WatcherSetPtr p = data_watches_.TriggerWatcher(path, ET_NODE_DELETED);
      child_watches_.TriggerWatcher(path, ET_NODE_DELETED, std::move(p));
      child_watches_.TriggerWatcher(
          parent.empty() ? "/" : parent, ET_NODE_CHILDREN_CHANGED);
    } else {
      response->set_code(RC_NO_PARENT);
    }
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::SetData(const std::string& path, const std::string& data,
                       SetDataResponse* response) {
  MutexLock lock(&mutex_);
  auto it  =nodes_.find(path);
  if (it != nodes_.end()) {
    Stat* stat = new Stat();
    it->second->set_data(data);
    it->second->CopyStat(stat);
    response->set_allocated_stat(stat);
    data_watches_.TriggerWatcher(path, ET_NODE_DATA_CHANGED);
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::GetData(const std::string& path, Watcher* watcher,
                       GetDataResponse* response) {
  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    Stat* stat = new Stat();
    it->second->CopyStat(stat);
    response->set_data(it->second->data());
    response->set_allocated_stat(stat);
    if (watcher != nullptr) {
      data_watches_.AddWatch(path, watcher);
    }
  } else {
    response->set_code(RC_NO_NODE);
  }
}

}  // namespace saber
