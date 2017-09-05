// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_TREE_H_
#define SABER_SERVER_DATA_TREE_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "saber/proto/saber.pb.h"
#include "saber/proto/server.pb.h"
#include "saber/server/server_watch_manager.h"
#include "saber/util/mutex.h"

namespace saber {

class DataTree {
 public:
  DataTree();
  ~DataTree();

  uint64_t Recover(const std::string& s, size_t index);

  void Create(const CreateRequest& request, const Transaction& txn,
              CreateResponse* response);

  void Delete(const DeleteRequest& request, const Transaction& txn,
              DeleteResponse* response);

  void Exists(const ExistsRequest& request, Watcher* watcher,
              ExistsResponse* response);

  void GetData(const GetDataRequest& request, Watcher* watcher,
               GetDataResponse* response);

  void SetData(const SetDataRequest& request, const Transaction& txn,
               SetDataResponse* response);

  void GetACL(const GetACLRequest& request, GetACLResponse* response);

  void SetACL(const SetACLRequest& request, const Transaction& txn,
              SetACLResponse* response);

  void GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   GetChildrenResponse* response);

  void RemoveWatcher(Watcher* watcher);

  void KillSession(uint64_t session_id, const Transaction& txn);

  // Sync serialize
  void SerializeToString(std::string* data, size_t size);

  // No thread safe
  std::unordered_map<std::string, DataNode>* CopyNodes() const;

  // No thread safe
  std::unordered_map<std::string, std::set<std::string>>* CopyChildrens() const;

  // Async serialize
  static void SerializeToString(
      std::unordered_map<std::string, DataNode>& nodes,
      std::unordered_map<std::string, std::set<std::string>>& children,
      std::string* data, size_t size);

 private:
  Mutex mutex_;
  std::unordered_map<std::string, DataNode> nodes_;
  std::unordered_map<std::string, std::set<std::string>> childrens_;

  std::unordered_map<uint64_t, std::set<std::string>> ephemerals_;

  ServerWatchManager data_watches_;
  ServerWatchManager child_watches_;

  // No copying allowed
  DataTree(const DataTree&);
  void operator=(const DataTree&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_TREE_H_
