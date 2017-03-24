// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SERVER_MANAGER_H_
#define SABER_CLIENT_SERVER_MANAGER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "saber/util/mutex.h"

namespace saber {

class ServerManager {
 public:
  static ServerManager* Instance() {
    manager_ = new ServerManager();
    return manager_;
  }
  void UpdateServers(const std::string& servers);

  std::pair<std::string, uint16_t> GetNext();

 private:
  static ServerManager* manager_;

  Mutex mutex_;
  size_t next_;
  std::vector<std::pair<std::string, uint16_t> > servers_;

  // No copying allowed
  ServerManager();
  ServerManager(const ServerManager&);
  void operator=(const ServerManager&);
};

ServerManager* ServerManager::manager_ = nullptr;

}  // namespace saber

#endif  // SABER_CLIENT_SERVER_MANAGER_H_
