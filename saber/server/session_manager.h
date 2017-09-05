// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SESSION_MANAGER_H_
#define SABER_SERVER_SESSION_MANAGER_H_

#include <string>
#include <unordered_map>

#include "saber/util/mutex.h"

namespace saber {

class SessionManager {
 public:
  SessionManager() {}
  ~SessionManager() {}

  uint64_t Recover(const std::string& s, size_t index);

  bool FindSession(uint64_t session_id, uint64_t* version) const;

  bool FindSession(uint64_t session_id, uint64_t version) const;

  bool CreateSession(uint64_t session_id, uint64_t new_version,
                     uint64_t old_version);

  bool CloseSession(uint64_t session_id, uint64_t version);

  // Sync serialize
  void SerializeToString(std::string* s);

  // No thread safe
  std::unordered_map<uint64_t, uint64_t>* CopySessions() const;

  // Async serialize
  static void SerializeToString(
      const std::unordered_map<uint64_t, uint64_t>& sessions, std::string* s);

 private:
  mutable Mutex mutex_;
  std::unordered_map<uint64_t, uint64_t> sessions_;

  // No copying allowed
  SessionManager(const SessionManager&);
  void operator=(const SessionManager&);
};

}  // namespace saber

#endif  // SABER_SERVER_SESSION_MANAGER_H_
