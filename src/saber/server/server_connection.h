// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_CONNECTION_H_
#define SABER_SERVER_SERVER_CONNECTION_H_

#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_connection.h>

#include "saber/server/data_base.h"
#include "saber/service/watcher.h"
#include "saber/proto/saber.pb.h"

namespace saber {

class ServerConnection : public Watcher {
 public:
  ServerConnection(const voyager::TcpConnectionPtr& p, DataBase* db);
  virtual ~ServerConnection();

  virtual void Process(const WatchedEvent& event);

  void SetSessionId(uint64_t id) { session_id_ = id; }
  uint64_t SessionId() const { return session_id_; }

  void OnMessage(voyager::Buffer* buf);

 private:
  static const int kHeaderSize = 4;

  void HandleMessage(const SaberMessage& msg);

  uint64_t session_id_;
  voyager::TcpConnectionPtr conn_ptr_;
  voyager::EventLoop* loop_;

  DataBase* db_;

  // No copying allowed
  ServerConnection(const ServerConnection&);
  void operator=(const ServerConnection&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_CONNECTION_H_
