// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/session_manager.h"

#include <assert.h>
#include <voyager/util/coding.h>

#include "saber/util/mutexlock.h"

namespace saber {

uint64_t SessionManager::Recover(const std::string& s, size_t index) {
  const char* p = s.c_str();
  p += index;
  uint64_t size = voyager::DecodeFixed64(p);
  p += 8;
  index += 8;
  uint64_t all = index + size;
  assert(all <= s.size());
  while (index < all) {
    uint64_t session_id = voyager::DecodeFixed64(p);
    uint64_t instance_id = voyager::DecodeFixed64(p + 8);
    sessions_.insert(std::make_pair(session_id, instance_id));
    p += 16;
    index += 16;
  }
  assert(index == all);
  assert(size == 16 * sessions_.size());
  return index;
}

bool SessionManager::FindSession(uint64_t session_id, uint64_t* version) const {
  MutexLock lock(&mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    *version = it->second;
    return true;
  }
  return false;
}

bool SessionManager::FindSession(uint64_t session_id, uint64_t version) const {
  MutexLock lock(&mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    return it->second == version;
  }
  return false;
}

bool SessionManager::CreateSession(uint64_t session_id, uint64_t new_version,
                                   uint64_t old_version) {
  MutexLock lock(&mutex_);
  if (old_version != 0) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && it->second == old_version) {
      it->second = new_version;
      return true;
    } else {
      return false;
    }
  } else {
    sessions_[session_id] = new_version;
    return true;
  }
}

bool SessionManager::CloseSession(uint64_t session_id, uint64_t version) {
  MutexLock lock(&mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    if (it->second == version) {
      sessions_.erase(it);
      return true;
    }
    return false;
  }
  return true;
}

void SessionManager::SerializeToString(std::string* s) const {
  SessionManager::SerializeToString(sessions_, s);
}

std::unordered_map<uint64_t, uint64_t>* SessionManager::CopySessions() const {
  return new std::unordered_map<uint64_t, uint64_t>(sessions_);
}

void SessionManager::SerializeToString(
    const std::unordered_map<uint64_t, uint64_t>& sessions, std::string* s) {
  voyager::PutFixed64(s, 16 * sessions.size());
  for (auto& it : sessions) {
    voyager::PutFixed64(s, it.first);
    voyager::PutFixed64(s, it.second);
  }
}

}  // namespace saber
