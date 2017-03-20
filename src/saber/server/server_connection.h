// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_CONNECTION_H_
#define SABER_SERVER_SERVER_CONNECTION_H_

#include <voyager/core/tcp_connection.h>

#include "saber/service/watcher.h"

namespace saber {

class ServerConnection : public Watcher {
 public:
  ServerConnection(const voyager::TcpConnectionPtr& p);
  virtual ~ServerConnection();

  virtual void Process(const WatchedEvent& event);

 private:
  voyager::TcpConnectionPtr conn_ptr_;

  // No copying allowed
  ServerConnection(const ServerConnection&);
  void operator=(const ServerConnection&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_CONNECTION_H_
