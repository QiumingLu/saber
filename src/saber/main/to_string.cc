// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/main/to_string.h"

namespace saber {

std::string ToString(SessionState state) {
  std::string s;
  switch (state) {
    case SS_CONNECTED:
      s = "Connected";
      break;
    case SS_DISCONNECTED:
      s = "Disconnected";
      break;
    case SS_EXPIRED:
      s = "Expired";
      break;
    case SS_AUTHFAILED:
      s = "AuthFailed";
      break;
    default:
      s = "Unknown";
      break;
  }
  s += "\n";
  return s;
}

std::string ToString(EventType type) {
  std::string s;
  switch (type) {
    case ET_NONE:
      s = "None";
      break;
    case ET_NODE_CREATED:
      s = "Node Created";
      break;
    case ET_NODE_DELETED:
      s = "Node Deleted";
      break;
    case ET_NODE_DATA_CHANGED:
      s = "Node Data Changed";
      break;
    case ET_NODE_CHILDREN_CHANGED:
      s = "Node Children Changed";
      break;
    case ET_DATA_WATCH_REMOVED:
      s = "Data Watch Removed";
      break;
    case ET_CHILD_WATCH_REMOVED:
      s = "Child Watch Removed";
      break;
    default:
      s = "Unknown";
      assert(false);
      break;
  }
  s += "\n";
  return s;
}

std::string ToString(const WatchedEvent& event) {
  std::string s = "state: ";
  s += ToString(event.state());
  s += "event_type: ";
  s += ToString(event.type());
  s += "path: ";
  s += event.path();
  s += "\n";
  return s;
}

std::string ToString(ResponseCode code) {
  std::string s;
  switch (code) {
    case RC_OK:
      s = "OK\n";
      break;
    case RC_FAILED:
      s = "FAIL\n";
      break;
    case RC_NO_NODE:
      s = "NoNode\n";
      break;
    case RC_NO_PARENT:
      s = "NoParent\n";
      break;
    case RC_NODE_EXISTS:
      s = "NodeExists\n";
      break;
    case RC_BAD_VERSION:
      s = "BadVersion\n";
      break;
    case RC_NO_AUTH:
      s = "NoAuth\n";
      break;
    default:
      assert(false);
      break;
  }
  return s;
}

std::string ToString(const Stat& stat) {
  std::string s;
  s += "group_id: ";
  s += std::to_string(stat.group_id());
  s += "\n";
  s += "created_id: ";
  s += std::to_string(stat.created_id());
  s += "\n";
  s += "modified_id: ";
  s += std::to_string(stat.modified_id());
  s += "\n";
  s += "created_time: ";
  s += std::to_string(stat.created_time());
  s += "\n";
  s += "modified_time: ";
  s += std::to_string(stat.modified_time());
  s += "\n";
  s += "version: ";
  s += std::to_string(stat.version());
  s += "\n";
  s += "children_version: ";
  s += std::to_string(stat.children_version());
  s += "\n";
  s += "acl_version: ";
  s += std::to_string(stat.acl_version());
  s += "\n";
  s += "ephemeral_id: ";
  s += std::to_string(stat.ephemeral_id());
  s += "\n";
  s += "data_len: ";
  s += std::to_string(stat.data_len());
  s += "\n";
  s += "children_num: ";
  s += std::to_string(stat.children_num());
  s += "\n";
  s += "children_id: ";
  s += std::to_string(stat.children_id());
  s += "\n";
  s += "\n";
  return s;
}

std::string ToString(const CreateResponse& response) {
  std::string s = ToString(response.code());
  s += "path: ";
  s += response.path();
  s += "\n";
  return s;
}

std::string ToString(const DeleteResponse& response) {
  std::string s = ToString(response.code());
  return s;
}

std::string ToString(const ExistsResponse& response) {
  std::string s = ToString(response.code());
  s += "stat:\n";
  s += ToString(response.stat());
  return s;
}

std::string ToString(const GetDataResponse& response) {
  std::string s = ToString(response.code());
  s += "data:\n";
  s += response.data();
  s += "\n";
  s += "stat:\n";
  s += ToString(response.stat());
  return s;
}

std::string ToString(const SetDataResponse& response) {
  std::string s = ToString(response.code());
  s += "stat:\n";
  s += ToString(response.stat());
  return s;
}

std::string ToString(const GetACLResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const SetACLResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const GetChildrenResponse& response) {
  std::string s = ToString(response.code());
  s += "stat:\n";
  s += ToString(response.stat());
  s += "children:\n";
  for (int i = 0; i < response.children_size(); ++i) {
    printf("%d:%s\n", i+1, response.children(i).c_str());
  }
  return s;
}

}  // namespace saber
