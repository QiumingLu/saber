// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_CONNECTION_MONITOR_H_
#define SABER_SERVER_CONNECTION_MONITOR_H_

#include <string>
#include <map>
#include <unordered_map>

#include <voyager/core/tcp_connection.h>
#include <skywalker/node.h>

namespace saber {

class SaberDB;
class ServerConnection;

class ConnectionMonitor {
 public:
  ConnectionMonitor(int server_id,
                    int max_ip_connections, int max_all_connections,
                    SaberDB* db, skywalker::Node* node);
  ~ConnectionMonitor();

  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);

 private:
  uint64_t GetNextSessionId() const;

  const int server_id_;
  const int max_ip_connections_;
  const int max_all_connections_;
  SaberDB* db_;
  skywalker::Node* node_;

  std::map<std::string, int> ip_counter_;
  std::unordered_map<uint64_t, std::unique_ptr<ServerConnection> > conns_;

  // No copying allowed
  ConnectionMonitor(const ConnectionMonitor&);
  void operator=(const ConnectionMonitor&);
};

}  // namespace saber

#endif  // SABER_SERVER_CONNECTION_MONITOR_H_
