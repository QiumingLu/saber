// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_TREE_H_
#define SABER_SERVER_DATA_TREE_H_

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "saber/proto/saber.pb.h"
#include "saber/proto/server.pb.h"
#include "saber/server/server_watch_manager.h"

namespace saber {

class DataTree {
 public:
  DataTree();
  ~DataTree();

  void Recover(const DataNodeList& node_list);
  DataNodeList GetDataNodeList() const;

  void Create(const CreateRequest& request, const Transaction* txn,
              CreateResponse* response, bool only_check = false);

  void Delete(const DeleteRequest& request, const Transaction* txn,
              DeleteResponse* response, bool only_check = false);

  void Exists(const ExistsRequest& request, Watcher* watcher,
              ExistsResponse* response);

  void GetData(const GetDataRequest& request, Watcher* watcher,
               GetDataResponse* response);

  void SetData(const SetDataRequest& request, const Transaction* txn,
               SetDataResponse* response, bool only_check = false);

  void GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   GetChildrenResponse* response);

  void RemoveWatcher(Watcher* watcher);

  void KillSession(uint64_t session_id, const Transaction* txn);

 private:
  ResponseCode ParsePath(const std::string& path,
                         std::string* parent, std::string* child) const;
  std::mutex mutex_;
  std::unordered_map<std::string, DataNode> nodes_;
  std::unordered_map<std::string, std::unordered_set<std::string>> childrens_;
  std::unordered_map<uint64_t, std::unordered_set<std::string>> ephemerals_;

  ServerWatchManager data_watches_;
  ServerWatchManager child_watches_;

  // No copying allowed
  DataTree(const DataTree&);
  void operator=(const DataTree&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_TREE_H_
