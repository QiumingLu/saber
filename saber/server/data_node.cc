// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/data_node.h"

namespace saber {

DataNode::DataNode() {}

DataNode::~DataNode() {}

bool DataNode::AddChild(const std::string& child, uint64_t children_id) {
  auto it = children_.insert(child);
  bool res = it.second;
  if (res) {
    UpdateChildrenStat(children_id);
  }
  return res;
}

bool DataNode::RemoveChild(const std::string& child, uint64_t children_id) {
  bool res = children_.erase(child);
  if (res) {
    UpdateChildrenStat(children_id);
  }
  return res;
}

void DataNode::UpdateChildrenStat(uint64_t children_id) {
  stat_.set_children_version(stat_.children_version() + 1);
  stat_.set_children_num(static_cast<int>(children_.size()));
  stat_.set_children_id(children_id);
}

}  // namespace saber
