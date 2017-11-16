// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/server_manager_impl.h"

#include <assert.h>
#include <unistd.h>

#include <random>

#include <voyager/util/string_util.h>

#include "saber/util/timeops.h"

namespace saber {

void ServerManagerImpl::UpdateServers(const std::string& servers) {
  std::vector<std::string> v;
  voyager::SplitStringUsing(servers, ",", &v);
  servers_.clear();
  for (auto& s : v) {
    size_t found = s.find_first_of(":");
    if (found != std::string::npos) {
      uint16_t port = static_cast<uint16_t>(atoi(s.substr(found + 1).c_str()));
      servers_.push_back(voyager::SockAddr(s.substr(0, found), port));
    }
  }
  std::shuffle(servers_.begin(), servers_.end(),
               std::default_random_engine(NowMicros()));
  assert(!servers_.empty());
  next_ = 0;
}

size_t ServerManagerImpl::GetSize() const { return servers_.size(); }

voyager::SockAddr ServerManagerImpl::GetNext() {
  assert(!servers_.empty());
  if (next_ >= servers_.size()) {
    next_ = 0;
  }
  return servers_[next_++];
}

void ServerManagerImpl::OnConnection() {}

}  // namespace saber
