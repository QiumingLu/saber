// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_node.h"

namespace saber {

DataNode::DataNode() {
}

DataNode::DataNode(
    const Stat& stat, const std::string& data, const std::vector<ACL>& acl)
    : stat_(stat),
      data_(data),
      acl_(acl) {
}

DataNode::DataNode(
    const Stat& stat, const std::string& data, std::vector<ACL>&& acl)
    : stat_(stat),
      data_(data),
      acl_(std::move(acl)) {
}

DataNode::~DataNode() {
}

bool DataNode::AddChild(const std::string& child) {
  auto it = children_.insert(child);
  return it.second;
}

bool DataNode::RemoveChild(const std::string& child) {
  return children_.erase(child);
}

}  // namespace saber
