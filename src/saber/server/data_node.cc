// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_node.h"

namespace saber {

DataNode::DataNode() {
}

DataNode::DataNode(const std::string& data)
    : data_(data) {
}

DataNode::~DataNode() {
}

void DataNode::CopyStat(Stat* stat) const {
  *stat = stat_;
}

bool DataNode::AddChild(const std::string& child) {
  auto it = children_.insert(child);
  return it.second;
}

bool DataNode::RemoveChild(const std::string& child) {
  return children_.erase(child);
}

}  // namespace saber
