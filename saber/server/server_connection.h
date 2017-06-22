// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_CONNECTION_H_
#define SABER_SERVER_SERVER_CONNECTION_H_

#include <memory>
#include <queue>

#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_connection.h>

#include "saber/net/messager.h"
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

  void OnMessage(const voyager::TcpConnectionPtr& p, voyager::Buffer* buf);
  void OnCommitComplete(std::unique_ptr<SaberMessage> message);

  virtual void Process(const WatchedEvent& event);

 private:
  bool HandleMessage(std::unique_ptr<SaberMessage> message);

  bool closed_;
  bool last_finished_;
  const uint64_t session_id_;
  voyager::TcpConnectionPtr conn_;
  SaberDB* db_;
  std::unique_ptr<Messager> messager_;
  std::queue<std::unique_ptr<SaberMessage> > pending_messages_;
  CommitterPtr committer_;

  // No copying allowed
  ServerConnection(const ServerConnection&);
  void operator=(const ServerConnection&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_CONNECTION_H_
