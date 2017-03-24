// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SERVER_MANAGER_IMPL_H_
#define SABER_CLIENT_SERVER_MANAGER_IMPL_H_

#include <vector>

#include "saber/client/server_manager.h"
#include "saber/util/mutex.h"

namespace saber {

class ServerManagerImpl : public ServerManager {
 public:
  ServerManagerImpl();

  virtual void UpdateServers(const std::string& servers);

  virtual std::pair<std::string, uint16_t> GetNext();

 private:
  Mutex mutex_;
  size_t next_;
  std::vector<std::pair<std::string, uint16_t> > servers_;

  // No copying allowed
  ServerManagerImpl(const ServerManagerImpl&);
  void operator=(const ServerManagerImpl&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SERVER_MANAGER_IMPL_H_
