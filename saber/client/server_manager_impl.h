// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SERVER_MANAGER_IMPL_H_
#define SABER_CLIENT_SERVER_MANAGER_IMPL_H_

#include <string>
#include <vector>

#include "saber/client/server_manager.h"

namespace saber {

class ServerManagerImpl : public ServerManager {
 public:
  ServerManagerImpl() : next_(0) {}
  virtual ~ServerManagerImpl() {}

  virtual void UpdateServers(const std::string& servers);

  virtual size_t GetSize() const;

  virtual voyager::SockAddr GetNext();

  virtual void OnConnection();

 private:
  size_t next_;
  std::vector<voyager::SockAddr> servers_;

  // No copying allowed
  ServerManagerImpl(const ServerManagerImpl&);
  void operator=(const ServerManagerImpl&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SERVER_MANAGER_IMPL_H_
