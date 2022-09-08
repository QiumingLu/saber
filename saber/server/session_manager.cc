// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/session_manager.h"

#include <assert.h>

#include "saber/util/coding.h"

namespace saber {

void SessionManager::Recover(const SessionList& session_list) {
  for (const auto& session : session_list.sessions()) {
    sessions_[session.session_id()] = session.version();
  }
}

SessionList SessionManager::GetSessionList() const {
  SessionList session_list;
  for (const auto& it : sessions_) {
    auto session = session_list.add_sessions();
    session->set_session_id(it.first);
    session->set_version(it.second);
  }
  return session_list;
}


bool SessionManager::FindSession(uint64_t session_id, uint64_t* version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    *version = it->second;
    return true;
  }
  return false;
}

bool SessionManager::FindSession(uint64_t session_id, uint64_t version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    return it->second == version;
  }
  return false;
}

bool SessionManager::CreateSession(uint64_t session_id, uint64_t new_version,
                                   uint64_t old_version) {
  std::lock_guard<std::mutex> lock(mutex_);
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
  std::lock_guard<std::mutex> lock(mutex_);
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

std::unordered_map<uint64_t, uint64_t> SessionManager::CopySessions() const {
  return std::unordered_map<uint64_t, uint64_t>(sessions_);
}

}  // namespace saber
