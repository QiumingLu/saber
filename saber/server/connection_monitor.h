// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_CONNECTION_MONITOR_H_
#define SABER_SERVER_CONNECTION_MONITOR_H_

#include <map>
#include <string>

#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_connection.h>

#include "saber/server/connection_monitor.h"
#include "saber/util/mutex.h"

namespace saber {

class ConnectionMonitor {
 public:
  ConnectionMonitor(int max_ip_connections);
  ~ConnectionMonitor();

  bool OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);

 private:
  const int max_ip_connections_;

  Mutex mutex_;
  std::map<std::string, int> ip_counter_;

  // No copying allowed
  ConnectionMonitor(const ConnectionMonitor&);
  void operator=(const ConnectionMonitor&);
};

}  // namespace saber

#endif  // SABER_SERVER_CONNECTION_MONITOR_H_
