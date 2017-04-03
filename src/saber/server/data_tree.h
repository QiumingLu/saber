// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_TREE_H_
#define SABER_SERVER_DATA_TREE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "saber/server/data_node.h"
#include "saber/server/server_watch_manager.h"
#include "saber/util/mutex.h"

namespace saber {

class DataTree {
 public:
  DataTree();
  ~DataTree();

  void Create(const CreateRequest& request,
              CreateResponse* response);
 
  void Delete(const DeleteRequest& request,
              DeleteResponse* response);

  void Exists(const ExistsRequest& request, Watcher* watcher,
              ExistsResponse* response);
 
  void GetData(const GetDataRequest& request, Watcher* watcher,
               GetDataResponse* response);

  void SetData(const SetDataRequest& request,
               SetDataResponse* response);
 
  void GetACL(const GetACLRequest& request,
              GetACLResponse* response);
 
 void SetACL(const SetACLRequest& request, 
              SetACLResponse* response); 

  void GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   GetChildrenResponse* response);
 private:
  Mutex mutex_;
  ServerWatchManager data_watches_;
  ServerWatchManager child_watches_;
  std::unordered_map<std::string, std::unique_ptr<DataNode> > nodes_;

  // No copying allowed
  DataTree(const DataTree&);
  void operator=(const DataTree&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_TREE_H_
