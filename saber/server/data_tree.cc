// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_tree.h"

#include <utility>
#include <vector>

#include "saber/util/coding.h"
#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"

namespace saber {

static inline void AppendToString(std::string* s, size_t value) {
  PutFixed32(s, static_cast<uint32_t>(value));
}

DataTree::DataTree() { nodes_.insert(std::make_pair("", DataNode())); }

DataTree::~DataTree() {}

uint64_t DataTree::Recover(const std::string& s, size_t index) {
  const char* base = s.c_str();
  const char* p = base + index;

  uint32_t size = DecodeFixed32(p);
  p += 4;

  for (uint32_t i = 0; i < size; ++i) {
    uint32_t len = DecodeFixed32(p);
    p += 4;
    std::string name(p, len);
    p += len;

    DataNode& node = nodes_[name];

    len = DecodeFixed32(p);
    p += 4;
    node.ParseFromArray(p, len);
    p += len;

    len = DecodeFixed32(p);
    p += 4;

    if (len > 0) {
      std::unordered_set<std::string>& children = childrens_[name];
      for (uint32_t idx = 0; idx < len; ++idx) {
        uint32_t temp = DecodeFixed32(p);
        p += 4;
        children.insert(std::string(p, temp));
        p += temp;
      }
    }

    if (node.stat().ephemeral_id() != 0) {
      ephemerals_[node.stat().ephemeral_id()].insert(name);
    }
  }

  return (p - base);
}

void DataTree::Create(const CreateRequest& request, const Transaction* txn,
                      CreateResponse* response, bool only_check) {
  std::string path = request.path();
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);

  if (parent == "" && (request.type() == NT_PERSISTENT_SEQUENTIAL ||
                       request.type() == NT_EPHEMERAL_SEQUENTIAL)) {
    response->set_code(RC_NO_PARENT);
    return;
  }

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(parent);
    if (it == nodes_.end()) {
      response->set_code(RC_NO_PARENT);
      return;
    }

    // TODO
    if (!CheckACL(it->second, kCreate, nullptr)) {
      response->set_code(RC_NO_AUTH);
      return;
    }
    bool b = false;
    if (request.type() == NT_PERSISTENT_SEQUENTIAL ||
        request.type() == NT_EPHEMERAL_SEQUENTIAL) {
      b = true;
      if (!only_check) {
        char seq[16];
        snprintf(seq, sizeof(seq), "_%010d",
                 it->second.stat().children_version() + 1);
        child.append(seq);
        path.append(seq);
      }
    }
    std::unordered_set<std::string>& children = childrens_[parent];
    if (!b && children.find(child) != children.end()) {
      response->set_code(RC_NODE_EXISTS);
    } else if (only_check) {
      response->set_code(RC_OK);
    } else {
      children.insert(child);
      Stat* tmp = it->second.mutable_stat();
      tmp->set_children_version(tmp->children_version() + 1);
      tmp->set_children_num(static_cast<uint32_t>(children.size()));
      tmp->set_children_id(txn->instance_id());
      DataNode& node = nodes_[path];
      Stat* stat = node.mutable_stat();
      stat->set_group_id(txn->group_id());
      stat->set_created_id(txn->instance_id());
      stat->set_modified_id(txn->instance_id());
      stat->set_created_time(txn->time());
      stat->set_modified_time(txn->time());
      stat->set_version(0);
      stat->set_children_version(0);
      stat->set_acl_version(0);
      stat->set_data_len(static_cast<uint32_t>(request.data().size()));
      stat->set_children_num(0);
      stat->set_children_id(txn->instance_id());
      node.set_data(request.data());
      *(node.mutable_acl()) = request.acl();
      if (request.type() == NT_EPHEMERAL ||
          request.type() == NT_EPHEMERAL_SEQUENTIAL) {
        stat->set_ephemeral_id(txn->session_id());
        ephemerals_[stat->ephemeral_id()].insert(path);
      }
      response->set_code(RC_OK);
      response->set_path(path);
    }
  }
  if (!only_check && response->code() == RC_OK) {
    data_watches_.TriggerWatcher(path, ET_NODE_CREATED);
    child_watches_.TriggerWatcher(parent.empty() ? "/" : parent,
                                  ET_NODE_CHILDREN_CHANGED);
  }
}

void DataTree::Delete(const DeleteRequest& request, const Transaction* txn,
                      DeleteResponse* response, bool only_check) {
  const std::string& path = request.path();
  size_t found = path.find_last_of('/');
  std::string parent = path.substr(0, found);
  std::string child = path.substr(found + 1);

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it == nodes_.end()) {
      response->set_code(RC_NO_NODE);
      return;
    }
    if (request.version() != -1 &&
        request.version() != it->second.stat().version()) {
      response->set_code(RC_BAD_VERSION);
      return;
    }

    auto p_it = nodes_.find(parent);
    // TODO
    if (p_it != nodes_.end() && !CheckACL(p_it->second, kDelete, nullptr)) {
      response->set_code(RC_NO_AUTH);
      return;
    }
    if (only_check) {
      response->set_code(RC_OK);
      return;
    }

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
    if (p_it != nodes_.end()) {
      if (childrens_.find(parent) != childrens_.end()) {
        std::unordered_set<std::string>& children = childrens_[parent];
        if (children.erase(child)) {
          Stat* tmp = p_it->second.mutable_stat();
          tmp->set_children_version(tmp->children_version() + 1);
          tmp->set_children_num(static_cast<uint32_t>(children.size()));
          tmp->set_children_id(txn->instance_id());
        }
        if (children.empty()) {
          childrens_.erase(parent);
        }
      }
      response->set_code(RC_OK);
    } else {
      response->set_code(RC_NO_PARENT);
    }
  }

  WatcherSetPtr p = data_watches_.TriggerWatcher(path, ET_NODE_DELETED);
  child_watches_.TriggerWatcher(path, ET_NODE_DELETED, std::move(p));
  child_watches_.TriggerWatcher(parent.empty() ? "/" : parent,
                                ET_NODE_CHILDREN_CHANGED);
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
    *(response->mutable_stat()) = it->second.stat();
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
      // TODO
      if (CheckACL(it->second, kRead, nullptr)) {
        response->set_code(RC_OK);
        response->set_data(it->second.data());
        *(response->mutable_stat()) = it->second.stat();
      } else {
        response->set_code(RC_NO_AUTH);
      }
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

void DataTree::SetData(const SetDataRequest& request, const Transaction* txn,
                       SetDataResponse* response, bool only_check) {
  const std::string& path = request.path();
  const std::string& data = request.data();

  {
    MutexLock lock(&mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      int version = it->second.stat().version();
      if (request.version() != -1 && request.version() != version) {
        response->set_code(RC_BAD_VERSION);
      } else if (!CheckACL(it->second, kWrite, nullptr)) {
        response->set_code(RC_NO_AUTH);
      } else if (only_check) {
        response->set_code(RC_OK);
      } else {
        Stat* stat = it->second.mutable_stat();
        stat->set_modified_id(txn->instance_id());
        stat->set_modified_time(txn->time());
        stat->set_version(version + 1);
        stat->set_data_len(static_cast<int>(data.size()));
        it->second.set_data(data);
        response->set_code(RC_OK);
        *(response->mutable_stat()) = *stat;
      }
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (!only_check && response->code() == RC_OK) {
    data_watches_.TriggerWatcher(path, ET_NODE_DATA_CHANGED);
  }
}

void DataTree::GetACL(const GetACLRequest& request, GetACLResponse* response) {
  const std::string& path = request.path();
  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    response->set_code(RC_OK);
    *(response->mutable_stat()) = it->second.stat();
    *(response->mutable_acl()) = it->second.acl();
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::SetACL(const SetACLRequest& request, const Transaction* txn,
                      SetACLResponse* response, bool only_check) {
  const std::string& path = request.path();

  MutexLock lock(&mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    int version = it->second.stat().acl_version();
    if (request.version() != -1 && request.version() != version) {
      response->set_code(RC_BAD_VERSION);
    } else if (!CheckACL(it->second, kAdmin, nullptr)) {
      response->set_code(RC_NO_AUTH);
    } else if (only_check) {
      response->set_code(RC_OK);
    } else {
      *(it->second.mutable_acl()) = request.acl();
      it->second.mutable_stat()->set_acl_version(version + 1);
      response->set_code(RC_OK);
      *(response->mutable_stat()) = it->second.stat();
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
      // TODO
      if (CheckACL(it->second, kRead, nullptr)) {
        response->set_code(RC_OK);
        *(response->mutable_stat()) = it->second.stat();
        if (childrens_.find(path) != childrens_.end()) {
          const std::unordered_set<std::string>& children = childrens_[path];
          for (auto& i : children) {
            response->add_children(i);
          }
        }
      } else {
        response->set_code(RC_NO_AUTH);
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

// TODO
bool DataTree::CheckACL(const DataNode& node, Permissions perm,
                        const std::vector<Id>* ids) {
  if (kSkipACL) {
    return true;
  }
  if (node.acl_size() == 0) {
    return true;
  }
  for (auto& id : *ids) {
    if (id.scheme() == "super") {
      return true;
    }
  }
  for (int i = 0; i < node.acl_size(); ++i) {
    const ACL& a = node.acl(i);
    if ((a.perms() & perm) != 0) {
      if (a.id().scheme() == "world" && a.id().id() == "anyone") {
        return true;
      }
    }
  }
  return false;
}

void DataTree::RemoveWatcher(Watcher* watcher) {
  data_watches_.RemoveWatcher(watcher);
  child_watches_.RemoveWatcher(watcher);
}

void DataTree::KillSession(uint64_t session_id, const Transaction* txn) {
  auto it = ephemerals_.find(session_id);
  if (it != ephemerals_.end()) {
    std::unordered_set<std::string> paths;
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

void DataTree::SerializeToString(std::string* s) const {
  SerializeToString(nodes_, childrens_, s);
}

std::unordered_map<std::string, DataNode>* DataTree::CopyNodes() const {
  return new std::unordered_map<std::string, DataNode>(nodes_);
}

std::unordered_map<std::string, std::unordered_set<std::string>>*
DataTree::CopyChildrens() const {
  return new std::unordered_map<std::string, std::unordered_set<std::string>>(
      childrens_);
}

void DataTree::SerializeToString(
    const std::unordered_map<std::string, DataNode>& nodes,
    const std::unordered_map<std::string, std::unordered_set<std::string>>&
        childrens,
    std::string* s) {
  AppendToString(s, nodes.size());
  for (auto& it : nodes) {
    AppendToString(s, it.first.size());
    s->append(it.first);
    AppendToString(s, it.second.ByteSizeLong());
    it.second.AppendToString(s);
    auto iter = childrens.find(it.first);
    if (iter != childrens.end()) {
      const std::unordered_set<std::string>& children = iter->second;
      AppendToString(s, children.size());
      for (auto& child : children) {
        AppendToString(s, child.size());
        s->append(child);
      }
    } else {
      AppendToString(s, 0);
    }
  }
}

}  // namespace saber
