// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_NODE_H_
#define SABER_SERVER_DATA_NODE_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "saber/proto/saber.pb.h"

namespace saber {

class DataNode {
 public:
  DataNode();
  ~DataNode();

  void set_stat(const Stat& stat) { stat_ = stat; }
  const Stat& stat() const { return stat_; }
  Stat* mutable_stat() { return &stat_; }

  void set_data(const std::string& data) { data_ = data; }
  void set_data(std::string&& data) { data_ = std::move(data); }
  const std::string& data() const { return data_; }
  std::string* mutable_data() { return &data_; }

  void set_acl(const std::vector<ACL>& acl) { acl_ = acl; }
  void set_acl(std::vector<ACL>&& acl) { acl_ = std::move(acl); }
  const std::vector<ACL>& acl() const { return acl_; }
  std::vector<ACL>* mutable_acl() { return &acl_; }

  const std::set<std::string>& children() const { return children_; }

  bool AddChild(const std::string& child, uint64_t children_id);
  bool RemoveChild(const std::string& child, uint64_t children_id);

 private:
  void UpdateChildrenStat(uint64_t children_id);

  Stat stat_;
  std::string data_;
  std::vector<ACL> acl_;
  std::set<std::string> children_;

  // No copying allowed
  DataNode(const DataNode&);
  void operator=(const DataNode&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_NODE_H_
