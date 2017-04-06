// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_cell.h"
#include "saber/util/mutexlock.h"

namespace saber {

OnceType SaberCell::once_ = SABER_ONCE_INIT;
SaberCell* SaberCell::instance_ = nullptr;

SaberCell::SaberCell() {
}

SaberCell::~SaberCell() {
  for (auto& i : id_to_servers_) {
    delete i.second;
  }
}

void SaberCell::AddServer(const ServerMessage& s) {
  MutexLock lock(&mutex_);
  if (id_to_servers_.find(s.server_id) == id_to_servers_.end()) {
    ServerMessage* server = new ServerMessage(s);
    id_to_servers_.insert(
        std::make_pair(s.server_id, server));
    client_to_servers_.insert(
        std::make_pair(IpPort(s.ip, s.client_port), server));
    paxos_to_servers_.insert(
        std::make_pair(IpPort(s.ip, s.paxos_port), server));
  }
}

void SaberCell::RemoveServer(uint16_t id) {
  MutexLock lock(&mutex_);
  auto it = id_to_servers_.find(id);
  if (it != id_to_servers_.end()) {
    delete it->second;
    id_to_servers_.erase(it);
  }
}

bool SaberCell::FindServerById(uint16_t id, ServerMessage* s) const {
  MutexLock lock(&mutex_);
  auto it = id_to_servers_.find(id);
  if (it != id_to_servers_.end()) {
    *s = *(it->second);
    return true;
  }
  return false;
}

bool SaberCell::FindServerByClientIpPort(
    const IpPort& i, ServerMessage* s) const {
  MutexLock lock(&mutex_);
  auto it = client_to_servers_.find(i);
  if (it != client_to_servers_.end()) {
    *s = *(it->second);
    return true;
  }
  return false;
}

bool SaberCell::FindServerByPaxosIpPort(
    const IpPort& i, ServerMessage* s) const {
  MutexLock lock(&mutex_);
  auto it = paxos_to_servers_.find(i);
  if (it != paxos_to_servers_.end()) {
    *s = *(it->second);
    return true;
  }
  return false;
}

}  // namespace saber
