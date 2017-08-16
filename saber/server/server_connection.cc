// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_connection.h"

#include <utility>

#include "saber/net/messager.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(uint64_t session_id,
                                   const voyager::TcpConnectionPtr& p,
                                   SaberDB* db, skywalker::Node* node)
    : closed_(false),
      last_finished_(true),
      session_id_(session_id),
      conn_wp_(p),
      db_(db),
      committer_(new Committer(this, p->OwnerEventLoop(), db, node)) {}

ServerConnection::~ServerConnection() { db_->RemoveWatcher(this); }

void ServerConnection::SetTcpConnection(const voyager::TcpConnectionPtr& p) {
  conn_wp_ = p;
  committer_->SetEventLoop(p->OwnerEventLoop());
}

bool ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  if (closed_) {
    return false;
  }
  if (message->type() != MT_PING) {
    pending_messages_.push(std::move(message));
    if (last_finished_) {
      last_finished_ = false;
      committer_->Commit(pending_messages_.front().get());
    }
  }
  return true;
}

void ServerConnection::OnCommitComplete(std::unique_ptr<SaberMessage> message) {
  Messager::SendMessage(conn_wp_.lock(), *message);
  assert(!pending_messages_.empty());
  std::unique_ptr<SaberMessage> front = std::move(pending_messages_.front());
  pending_messages_.pop();
  if (front->type() == MT_CLOSE_SESSION) {
  } else if (message->type() != MT_MASTER) {
    if (!pending_messages_.empty() && !closed_) {
      assert(!last_finished_);
      committer_->Commit(pending_messages_.front().get());
    } else {
      last_finished_ = true;
    }
  } else {
    ShutDown();
  }
}

void ServerConnection::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  voyager::TcpConnectionPtr p = conn_wp_.lock();
  if (p) {
    Messager::SendMessage(p, message);
  }
}

void ServerConnection::ShutDown() {
  closed_ = true;
  voyager::TcpConnectionPtr p = conn_wp_.lock();
  if (p) {
    p->ShutDown();
  }
}

}  // namespace saber
