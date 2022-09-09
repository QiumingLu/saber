// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_tree.h"

#include <utility>
#include <vector>

#include "saber/util/logging.h"

namespace saber {

DataTree::DataTree() { nodes_.insert(std::make_pair("", DataNode())); }

DataTree::~DataTree() {}

void DataTree::Recover(const DataNodeList& node_list) {
  for (const auto& node : node_list.nodes()) {
    auto& new_node = nodes_[node.path()];
    new_node.set_type(node.type());
    new_node.mutable_stat()->CopyFrom(node.stat());
    new_node.set_data(node.data());

    for (const auto& child : node.children()) {
      childrens_[node.path()].insert(child);
    }

    if (node.stat().ephemeral_id() > 0) {
      ephemerals_[node.stat().ephemeral_id()].insert(node.path());
    }
  }
}

DataNodeList DataTree::GetDataNodeList() const {
  DataNodeList node_list;
  for (const auto& it : nodes_) {
    auto node = node_list.add_nodes();
    node->CopyFrom(it.second);
    node->set_path(it.first);
    auto iter = childrens_.find(it.first);
    if (iter != childrens_.end()) {
      for (const auto& child : iter->second) {
        node->add_children(child);
      }
    }
  }
  return node_list;
}

ResponseCode DataTree::ParsePath(const std::string& path,
                                 std::string* parent, std::string* child) const {
  size_t found = path.find_last_of('/');
  if (found == std::string::npos) {
    return RC_NO_PARENT;
  }
  *parent = path.substr(0, found);
  *child = path.substr(found + 1);
  if (child->empty()) {
    return RC_FAILED;
  }
  return RC_OK;
}

void DataTree::Create(const CreateRequest& request, const Transaction* txn,
                      CreateResponse* response, bool only_check) {
  std::string path = request.path();
  std::string parent;
  std::string child;
  ResponseCode retcode = ParsePath(path, &parent, &child);
  if (retcode != RC_OK) {
    response->set_code(retcode);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(parent);
    if (it == nodes_.end()) {
      response->set_code(RC_NO_PARENT);
      return;
    }
    if (it->second.type() == NT_EPHEMERAL ||
        it->second.type() == NT_EPHEMERAL_SEQUENTIAL) {
      response->set_code(RC_PARENT_EPHEMERAL);
      return;
    }

    if (it->second.type() == NT_PERSISTENT_SEQUENTIAL ||
        it->second.type() == NT_EPHEMERAL_SEQUENTIAL) {
      if (!only_check) {
        char seq[16];
        snprintf(seq, sizeof(seq), "_%010d",
                 it->second.stat().children_version() + 1);
        child.append(seq);
        path.append(seq);
      }
    }
    std::unordered_set<std::string>& children = childrens_[parent];
    if (children.find(child) != children.end()) {
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
      stat->set_data_len(static_cast<uint32_t>(request.data().size()));
      stat->set_children_num(0);
      stat->set_children_id(txn->instance_id());
      node.set_data(request.data());
      node.set_type(request.node_type());
      if (request.node_type() == NT_EPHEMERAL ||
          request.node_type() == NT_EPHEMERAL_SEQUENTIAL) {
        stat->set_ephemeral_id(txn->session_id());
        ephemerals_[stat->ephemeral_id()].insert(path);
      }
      response->set_code(RC_OK);
      response->set_path(path);
    }
  }
  if (!only_check && response->code() == RC_OK) {
    data_watches_.TriggerWatcher(path, ET_NODE_CREATED);
    if (!parent.empty()) {
      child_watches_.TriggerWatcher(parent, ET_NODE_CHILDREN_CHANGED);
    }
  }
}

void DataTree::Delete(const DeleteRequest& request, const Transaction* txn,
                      DeleteResponse* response, bool only_check) {
  const std::string& path = request.path();
  std::string parent;
  std::string child;
  ResponseCode retcode = ParsePath(path, &parent, &child);
  if (retcode != RC_OK) {
    response->set_code(retcode);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
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
    if (childrens_.find(path) != childrens_.end()) {
      response->set_code(RC_CHILDREN_EXISTS);
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
    auto p_it = nodes_.find(parent);
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

  data_watches_.TriggerWatcher(path, ET_NODE_DELETED);
  if (!parent.empty()) {
    child_watches_.TriggerWatcher(parent, ET_NODE_CHILDREN_CHANGED);
  }
}

void DataTree::Exists(const ExistsRequest& request, Watcher* watcher,
                      ExistsResponse* response) {
  const std::string& path = request.path();
  std::string parent;
  std::string child;
  ResponseCode code = ParsePath(path, &parent, &child);
  if (code != RC_OK) {
    response->set_code(code);
    return;
  }
  if (watcher) {
    data_watches_.AddWatcher(path, watcher);
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto it = nodes_.find(path);
  if (it != nodes_.end()) {
    response->set_code(RC_OK);
    response->set_node_type(it->second.type());
    *(response->mutable_stat()) = it->second.stat();
  } else {
    response->set_code(RC_NO_NODE);
  }
}

void DataTree::GetData(const GetDataRequest& request, Watcher* watcher,
                       GetDataResponse* response) {
  const std::string& path = request.path();
  std::string parent;
  std::string child;
  ResponseCode code = ParsePath(path, &parent, &child);
  if (code != RC_OK) {
    response->set_code(code);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      response->set_code(RC_OK);
      response->set_node_type(it->second.type());
      response->set_data(it->second.data());
      *(response->mutable_stat()) = it->second.stat();
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
  std::string parent;
  std::string child;
  ResponseCode retcode = ParsePath(path, &parent, &child);
  if (retcode != RC_OK) {
    response->set_code(retcode);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      int version = it->second.stat().version();
      if (request.version() != -1 && request.version() != version) {
        response->set_code(RC_BAD_VERSION);
      } else if (only_check) {
        response->set_code(RC_OK);
      } else {
        Stat* stat = it->second.mutable_stat();
        stat->set_modified_id(txn->instance_id());
        stat->set_modified_time(txn->time());
        stat->set_version(version + 1);
        stat->set_data_len(static_cast<int>(request.data().size()));
        it->second.set_data(request.data());
        response->set_code(RC_OK);
        response->mutable_stat()->CopyFrom(*stat);
      }
    } else {
      response->set_code(RC_NO_NODE);
    }
  }

  if (!only_check && response->code() == RC_OK) {
    data_watches_.TriggerWatcher(path, ET_NODE_DATA_CHANGED);
  }
}

void DataTree::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                           GetChildrenResponse* response) {
  const std::string& path = request.path();
  std::string parent;
  std::string child;
  ResponseCode code = ParsePath(path, &parent, &child);
  if (code != RC_OK) {
    response->set_code(code);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(path);
    if (it != nodes_.end()) {
      response->set_code(RC_OK);
      *(response->mutable_stat()) = it->second.stat();
      if (childrens_.find(path) != childrens_.end()) {
        const std::unordered_set<std::string>& children = childrens_[path];
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

}  // namespace saber
