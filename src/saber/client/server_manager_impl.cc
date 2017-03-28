// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/server_manager_impl.h"

#include <assert.h>
#include <unistd.h>
#include <voyager/util/string_util.h>

#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"

namespace saber {

ServerManagerImpl::ServerManagerImpl() {
}

void ServerManagerImpl::UpdateServers(const std::string& servers) {
  MutexLock lock(&mutex_);
  std::vector<std::string> v;
  voyager::SplitStringUsing(servers, ",", &v);
  servers_.clear();
  for (auto& s : v) {
    std::vector<std::string> one;
    voyager::SplitStringUsing(s, ":", &one);
    assert(one.size() == 2);
    uint16_t port = static_cast<uint16_t>(atoi(one[1].data()));
    servers_.push_back(voyager::SockAddr(one[0], port));
  }
  assert(!servers_.empty());
  next_ = 0;
}

size_t ServerManagerImpl::GetSize() const {
  MutexLock lock(&mutex_);
  return servers_.size();
}

voyager::SockAddr ServerManagerImpl::GetNext() {
  MutexLock lock(&mutex_);
  assert(!servers_.empty());
  if (next_ >= servers_.size()) {
    next_ = 0;
  }
  return servers_[next_++];
}

void ServerManagerImpl::OnConnection() {
}

}  // namespace saber
