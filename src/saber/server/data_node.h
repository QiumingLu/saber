// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_NODE_H_
#define SABER_SERVER_DATA_NODE_H_

#include <set>
#include <string>

#include "saber/proto/saber.pb.h"

namespace saber {

class DataNode {
 public:
  DataNode();
  DataNode(const std::string& data);
  ~DataNode();

  void CopyStat(Stat* stat) const;

  void set_stat(const Stat& stat) { stat_ = stat; }
  const Stat& stat() const { return stat_; }
  Stat* mutable_stat() { return &stat_; }

  void set_data(const std::string& data) { data_ = data; }
  const std::string& data() const { return data_; }

  bool AddChild(const std::string& child);
  bool RemoveChild(const std::string& child);

 private:
  Stat stat_;
  std::string data_;
  std::set<std::string> children_;

  // No copying allowed
  DataNode(const DataNode&);
  void operator=(const DataNode&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_NODE_H_
