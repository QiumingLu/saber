// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_tree.h"

#include <voyager/util/coding.h>

#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"

namespace saber {

DataTree::DataTree() { nodes_.insert(std::make_pair("", DataNode())); }

DataTree::~DataTree() {}

uint64_t DataTree::Recover(const std::string& s, size_t index) {
  const char* p = s.c_str();
  p += index;
  uint64_t size = voyager::DecodeFixed64(p);
  p += 8;
  index += 8;
  uint64_t all = index + size;
  assert(all <= s.size());
  DataNode node;
  while (index < all) {
    uint32_t one = voyager::DecodeFixed32(p);
    node.ParseFromArray(p + 4, static_cast<int>(one));
    if (node.children_size() > 0) {
      std::set<std::string>& children = childrens_[node.name()];
      for (int i = 0; i < node.children_size(); ++i) {
        children.insert(node.children(i));
      }
      node.clear_children();
    }
    if (node.stat().ephemeral_id() != 0) {
      ephemerals_[node.stat().ephemeral_id()].insert(node.name());
    }
    nodes_[node.name()].Swap(&node);
    nodes_[node.name()].clear_name();
    p = p + 4 + one;
    index = index + 4 + one;
  }
  assert(index == all);
  return index;
}

void DataTree::Create(const CreateRequest& request, const Transaction& txn,
                      CreateResponse* response) {
  const std::string& path = request.path();
  const std::string& data = request.data();
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);

  DataNode node;
  Stat* stat = node.mutable_stat();
  stat->set_group_id(txn.group_id());
  stat->set_created_id(txn.instance_id());
  stat->set_modified_id(txn.instance_id());
  stat->set_created_time(txn.time());
  stat->set_modified_time(txn.time());
  stat->set_version(0);
  stat->set_children_version(0);
  stat->set_acl_version(0);
  stat->set_data_len(static_cast<uint32_t>(data.size()));
  stat->set_children_num(0);
  stat->set_children_id(txn.instance_id());
  node.set_data(request.data());
  *(node.mutable_acl()) = request.acl();
  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(parent);
    if (it != nodes_.end()) {
      std::set<std::string>& children = childrens_[parent];
      if (children.find(child) == children.end()) {
        children.insert(child);
        Stat* tmp = it->second.mutable_stat();
        tmp->set_children_version(tmp->children_version() + 1);
        tmp->set_children_num(static_cast<uint32_t>(children.size()));
        tmp->set_children_id(txn.instance_id());

        nodes_[path].Swap(&node);
        response->set_code(RC_OK);
        response->set_path(path);
      } else {
        response->set_code(RC_NODE_EXISTS);
      }
    } else {
      response->set_code(RC_NO_PARENT);
    }
  }
  if (response->code() == RC_OK) {
    if (request.type() == NT_EPHEMERAL ||
        request.type() == NT_EPHEMERAL_SEQUENTIAL) {
      stat->set_ephemeral_id(txn.session_id());
      ephemerals_[stat->ephemeral_id()].insert(path);
    }
    data_watches_.TriggerWatcher(path, ET_NODE_CREATED);
    child_watches_.TriggerWatcher(parent.empty() ? "/" : parent,
                                  ET_NODE_CHILDREN_CHANGED);
  }
}

void DataTree::Delete(const DeleteRequest& request, const Transaction& txn,
                      DeleteResponse* response) {
  const std::string& path = request.path();
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      if (it->second.stat().ephemeral_id() != 0) {
        auto e = ephemerals_.find(it->second.stat().ephemeral_id());
        if (e != ephemerals_.end()) {
          e->second.erase(path);
          if (e->second.empty()) {
            ephemerals_.erase(e);
          }
        }
      }
      nodes_.erase(it);
      it = nodes_.find(parent);
      if (it != nodes_.end()) {
        if (childrens_.find(parent) != childrens_.end()) {
          std::set<std::string>& children = childrens_[parent];
          if (children.erase(child)) {
            Stat* tmp = it->second.mutable_stat();
            tmp->set_children_version(tmp->children_version() + 1);
            tmp->set_children_num(static_cast<uint32_t>(children.size()));
            tmp->set_children_id(txn.instance_id());
          }
          if (children.empty()) {
            childrens_.erase(parent);
          }
        }
        response->set_code(RC_OK);
      } else {
        response->set_code(RC_NO_PARENT);
      }
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (response->code() == RC_OK) {
    WatcherSetPtr p = data_watches_.TriggerWatcher(path, ET_NODE_DELETED);
    // FIXME
    child_watches_.TriggerWatcher(path, ET_NODE_DELETED, std::move(p));
    child_watches_.TriggerWatcher(parent.empty() ? "/" : parent,
                                  ET_NODE_CHILDREN_CHANGED);
  }
}

void DataTree::Exists(const ExistsRequest& request, Watcher* watcher,
                      ExistsResponse* response) {
  const std::string& path = request.path();
  if (watcher) {
    data_watches_.AddWatcher(path, watcher);
  }

  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    response->set_code(RC_OK);
    Stat* stat = new Stat(it->second.stat());
    response->set_allocated_stat(stat);
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::GetData(const GetDataRequest& request, Watcher* watcher,
                       GetDataResponse* response) {
  const std::string& path = request.path();

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      response->set_code(RC_OK);
      Stat* stat = new Stat(it->second.stat());
      response->set_data(it->second.data());
      response->set_allocated_stat(stat);
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (response->code() == RC_OK) {
    if (watcher) {
      data_watches_.AddWatcher(path, watcher);
    }
  }
}

void DataTree::SetData(const SetDataRequest& request, const Transaction& txn,
                       SetDataResponse* response) {
  const std::string& path = request.path();
  const std::string& data = request.data();

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      int version = it->second.stat().version();
      if (request.version() != -1 && request.version() != version) {
        response->set_code(RC_BAD_VERSION);
      } else {
        Stat* stat = it->second.mutable_stat();
        stat->set_modified_id(txn.instance_id());
        stat->set_modified_time(txn.time());
        stat->set_version(version + 1);
        stat->set_data_len(static_cast<int>(data.size()));
        it->second.set_data(data);
        response->set_code(RC_OK);
        response->set_allocated_stat(new Stat(*stat));
      }
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (response->code() == RC_OK) {
    data_watches_.TriggerWatcher(path, ET_NODE_DATA_CHANGED);
  }
}

void DataTree::GetACL(const GetACLRequest& request, GetACLResponse* response) {
  const std::string& path = request.path();
  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    response->set_code(RC_OK);
    Stat* stat = new Stat(it->second.stat());
    response->set_allocated_stat(stat);
    *(response->mutable_acl()) = it->second.acl();
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::SetACL(const SetACLRequest& request, const Transaction& txn,
                      SetACLResponse* response) {
  const std::string& path = request.path();

  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    int version = it->second.stat().acl_version();
    if (request.version() != -1 && request.version() != version) {
      response->set_code(RC_BAD_VERSION);
    } else {
      *(it->second.mutable_acl()) = request.acl();
      it->second.mutable_stat()->set_acl_version(version + 1);
      response->set_code(RC_OK);
      Stat* stat = new Stat(it->second.stat());
      response->set_allocated_stat(stat);
    }
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                           GetChildrenResponse* response) {
  const std::string& path = request.path();

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      response->set_code(RC_OK);
      Stat* stat = new Stat(it->second.stat());
      response->set_allocated_stat(stat);
      if (childrens_.find(path) != childrens_.end()) {
        const std::set<std::string>& children = childrens_[path];
        for (auto& i : children) {
          response->add_children(i);
        }
      }
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (response->code() == RC_OK) {
    if (watcher) {
      child_watches_.AddWatcher(path, watcher);
    }
  }
}

void DataTree::RemoveWatcher(Watcher* watcher) {
  data_watches_.RemoveWatcher(watcher);
  child_watches_.RemoveWatcher(watcher);
}

void DataTree::KillSession(uint64_t session_id, const Transaction& txn) {
  auto it = ephemerals_.find(session_id);
  if (it != ephemerals_.end()) {
    std::set<std::string> paths;
    paths.swap(it->second);
    ephemerals_.erase(it);
    DeleteRequest request;
    DeleteResponse response;
    for (auto& p : paths) {
      request.set_path(p);
      request.set_version(-1);
      Delete(request, txn, &response);
      if (response.code() != RC_OK) {
        LOG_WARN(
            "Ignoring not RC_OK for path %s while removing ephemeral "
            "for dead session %llu.",
            p.c_str(), (unsigned long long)session_id);
      }
    }
  }
}

void DataTree::SerializeToString(std::string* s, size_t size) {
  DataTree::SerializeToString(nodes_, childrens_, s, size);
}

std::unordered_map<std::string, DataNode>* DataTree::CopyNodes() const {
  return new std::unordered_map<std::string, DataNode>(nodes_);
}

std::unordered_map<std::string, std::set<std::string>>*
DataTree::CopyChildrens() const {
  return new std::unordered_map<std::string, std::set<std::string>>(childrens_);
}

void DataTree::SerializeToString(
    std::unordered_map<std::string, DataNode>& nodes,
    std::unordered_map<std::string, std::set<std::string>>& childrens,
    std::string* s, size_t size) {
  size_t i = 0;
  size_t all = size + 8 + 4 * nodes.size();
  std::vector<uint32_t> v(nodes.size());
  for (auto& it : nodes) {
    it.second.set_name(it.first);
    auto children = childrens.find(it.first);
    if (children != childrens.end()) {
      for (auto& child : children->second) {
        it.second.add_children(child);
      }
    }
    v[i] = static_cast<uint32_t>(it.second.ByteSizeLong());
    all += v[i];
    ++i;
  }
  i = 0;
  s->reserve(all);
  voyager::PutFixed64(s, all - 8 - size);
  for (auto& it : nodes) {
    voyager::PutFixed32(s, v[i++]);
    it.second.AppendToString(s);
    it.second.clear_children();
    it.second.clear_name();
  }
}

}  // namespace saber
