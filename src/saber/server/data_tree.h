// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_TREE_H_
#define SABER_SERVER_DATA_TREE_H_

#include <memory>
#include <string>
#include <map>

#include "saber/server/data_node.h"
#include "saber/server/server_watch_manager.h"

namespace saber {

class DataTree {
 public:
  DataTree();
  ~DataTree();

  int CreateNode(const std::string& path, const std::string& data);
  int DeleteNode(const std::string& path);

  bool SetData(const std::string& path, const std::string& data);
  bool GetData(const std::string& path,
               Watcher* watcher, std::string* data);

 private:
  std::map<std::string, std::unique_ptr<DataNode> > nodes_;
  ServerWatchManager data_watches_;
  ServerWatchManager child_watches_;

  // No copying allowed
  DataTree(const DataTree&);
  void operator=(const DataTree&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_TREE_H_
