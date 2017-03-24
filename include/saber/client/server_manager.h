// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SERVER_MANAGER_H_
#define SABER_CLIENT_SERVER_MANAGER_H_

#include <stdint.h>
#include <string>

namespace saber {

class ServerManager {
 public:
  ServerManager() { }
  virtual ~ServerManager() { }

  virtual void UpdateServers(const std::string& servers) = 0;

  virtual std::pair<std::string, uint16_t> GetNext() = 0;

 private:
  // No copying allowed
  ServerManager(const ServerManager&);
  void operator=(const ServerManager&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SERVER_MANAGER_H_
