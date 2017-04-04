// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_SERVER_H_
#define SABER_SERVER_SABER_SERVER_H_

#include <memory>
#include <voyager/port/atomic_sequence_num.h>
#include <voyager/core/tcp_server.h>
#include <skywalker/node.h>

#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/util/concurrent_map.h"

namespace saber {

class SaberServer {
 public:
  SaberServer(uint16_t server_id,
              voyager::EventLoop* loop,
              const voyager::SockAddr& addr,
              int thread_size = 1);
  ~SaberServer();

  bool Start(const skywalker::Options& options);

 private:
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);
  uint64_t GetNextSessionId() const;

  uint16_t server_id_;

  std::unique_ptr<SaberDB> db_;
  std::unique_ptr<skywalker::Node> node_;

  ConcurrentMap<uint64_t, std::unique_ptr<ServerConnection> > conns_;
  voyager::TcpServer server_;

  // No copying allowed
  SaberServer(const SaberServer&);
  void operator=(const SaberServer&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_SERVER_H_
