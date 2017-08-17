// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_CONNECTION_H_
#define SABER_SERVER_SERVER_CONNECTION_H_

#include <deque>
#include <memory>

#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_connection.h>

#include "saber/proto/saber.pb.h"
#include "saber/server/committer.h"
#include "saber/server/saber_db.h"
#include "saber/service/watcher.h"

namespace saber {

class ServerConnection : public Watcher {
 public:
  ServerConnection(uint64_t session_id, const voyager::TcpConnectionPtr& p,
                   SaberDB* db, skywalker::Node* node);
  virtual ~ServerConnection();

  uint64_t session_id() const { return session_id_; }
  void SetTcpConnection(const voyager::TcpConnectionPtr& p);

  bool OnMessage(std::unique_ptr<SaberMessage> message);
  void OnCommitComplete(std::unique_ptr<SaberMessage> message);

  virtual void Process(const WatchedEvent& event);

 private:
  void ShutDown();

  bool closed_;
  bool last_finished_;
  const uint64_t session_id_;
  std::weak_ptr<voyager::TcpConnection> conn_wp_;
  SaberDB* db_;
  std::deque<std::unique_ptr<SaberMessage>> pending_messages_;
  CommitterPtr committer_;

  // No copying allowed
  ServerConnection(const ServerConnection&);
  void operator=(const ServerConnection&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_CONNECTION_H_
