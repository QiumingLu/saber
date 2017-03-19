// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_TREE_H_
#define SABER_SERVER_DATA_TREE_H_

#include <string>

#include "saber/util/concurrent_map.h"
#include "saber/server/data_node.h"
#include "saber/server/server_watch_manager.h"

namespace saber {

class DataTree {
 public:
  DataTree();
  ~DataTree();

  void CreateNode(const std::string& path, const std::string& data);
  void DeleteNode(const std::string& path);

 private:
  ConcurrentMap<std::string, DataNode*> nodes_;
  ServerWatchManager data_watches_;
  ServerWatchManager child_watches_;

  // No copying allowed
  DataTree(const DataTree&);
  void operator=(const DataTree&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_TREE_H_
