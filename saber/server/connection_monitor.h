// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_CONNECTION_MONITOR_H_
#define SABER_SERVER_CONNECTION_MONITOR_H_

#include <string>
#include <unordered_map>

#include <voyager/core/tcp_connection.h>

#include "saber/util/mutex.h"

namespace saber {

class ConnectionMonitor {
 public:
  ConnectionMonitor(int max_all_connections, int max_ip_connections);
  ~ConnectionMonitor();

  bool OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);

 private:
  const int kMaxAllConnections;
  const int kMaxIpConnections;

  Mutex mutex_;
  int counter_;
  std::unordered_map<std::string, int> ip_counter_;

  // No copying allowed
  ConnectionMonitor(const ConnectionMonitor&);
  void operator=(const ConnectionMonitor&);
};

}  // namespace saber

#endif  // SABER_SERVER_CONNECTION_MONITOR_H_
