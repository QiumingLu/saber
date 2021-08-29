// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/session_manager.h"

#include <assert.h>

#include "saber/util/coding.h"

namespace saber {

uint64_t SessionManager::Recover(const std::string& s, size_t index) {
  const char* base = s.c_str();
  const char* p = base + index;
  uint32_t size = DecodeFixed32(p);
  p += 4;
  for (uint32_t i = 0; i < size; ++i) {
    uint64_t session_id = DecodeFixed64(p);
    p += 8;
    uint64_t instance_id = DecodeFixed64(p);
    p += 8;
    sessions_.insert(std::make_pair(session_id, instance_id));
  }
  return (p - base);
}

bool SessionManager::FindSession(uint64_t session_id, uint64_t* version) const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    *version = it->second;
    return true;
  }
  return false;
}

bool SessionManager::FindSession(uint64_t session_id, uint64_t version) const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    return it->second == version;
  }
  return false;
}

bool SessionManager::CreateSession(uint64_t session_id, uint64_t new_version,
                                   uint64_t old_version) {
  std::unique_lock<std::mutex> lock(mutex_);
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
  std::unique_lock<std::mutex> lock(mutex_);
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
  PutFixed32(s, static_cast<uint32_t>(sessions.size()));
  for (auto& it : sessions) {
    PutFixed64(s, it.first);
    PutFixed64(s, it.second);
  }
}

}  // namespace saber
