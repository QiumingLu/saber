// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_CONNECTION_H_
#define SABER_SERVER_SERVER_CONNECTION_H_

#include <queue>
#include <voyager/core/eventloop.h>

#include "saber/server/saber_db.h"
#include "saber/server/committer.h"
#include "saber/service/watcher.h"
#include "saber/proto/saber.pb.h"
#include "saber/net/messager.h"

namespace saber {

class ServerConnection : public Watcher {
 public:
  ServerConnection(uint64_t session_id,
                   voyager::EventLoop* loop,
                   std::unique_ptr<Messager> p,
                   SaberDB* db,
                   skywalker::Node* node);
  virtual ~ServerConnection();

  virtual void Process(const WatchedEvent& event);

  uint64_t session_id() const { return session_id_; }

 private:
  void OnMessage(std::unique_ptr<SaberMessage> message);
  void OnCommitComplete(std::unique_ptr<SaberMessage> message);

  bool closed_;
  bool last_finished_;
  const uint64_t session_id_;
  voyager::EventLoop* loop_;
  std::queue<std::unique_ptr<SaberMessage> > pending_messages_;
  std::unique_ptr<Messager> messager_;
  CommitterPtr committer_;

  // No copying allowed
  ServerConnection(const ServerConnection&);
  void operator=(const ServerConnection&);
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_CONNECTION_H_
