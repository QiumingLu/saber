// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/server_manager_impl.h"

#include <assert.h>
#include <voyager/util/string_util.h>

#include "saber/util/mutexlock.h"

namespace saber {

ServerManagerImpl::ServerManagerImpl()
    : next_(0) {
}

void ServerManagerImpl::UpdateServers(const std::string& servers) {
  MutexLock lock(&mutex_);
  std::vector<std::string> v;
  voyager::SplitStringUsing(servers, ",", &v);
  for (auto& s : v) {
    std::vector<std::string> one;
    voyager::SplitStringUsing(s, ":", &one);
    assert(one.size() == 2);
    servers_.push_back(
        std::make_pair(one[0], static_cast<uint16_t>(atoi(one[1].data()))));
  }
  next_ = 0;
}

std::pair<std::string, uint16_t> ServerManagerImpl::GetNext() {
  MutexLock lock(&mutex_);
  assert(!servers_.empty());
  if (next_ >= servers_.size()) {
    next_ = 0;
  }
  return servers_[next_++];
}

}  // namespace saber
