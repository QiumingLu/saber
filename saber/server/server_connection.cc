// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_connection.h"

#include <utility>

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

void ServerConnection::Connect(const voyager::TcpConnectionPtr& p) {
  closed_ = false;
  pending_messages_.clear();
  conn_wp_ = p;
  committer_->SetEventLoop(p->OwnerEventLoop());
}

void ServerConnection::ShutDown() {
  closed_ = true;
  pending_messages_.clear();
  voyager::TcpConnectionPtr p = conn_wp_.lock();
  if (p) {
    p->ShutDown();
  }
}

bool ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  if (closed_) {
    return false;
  }
  if (message->type() != MT_PING) {
    pending_messages_.push_back(std::move(message));
    if (last_finished_) {
      last_finished_ = false;
      committer_->Commit(pending_messages_.front().get());
    }
  }
  return true;
}

void ServerConnection::OnCommitComplete(std::unique_ptr<SaberMessage> message) {
  assert(!pending_messages_.empty());
  message->set_id(pending_messages_.front()->id());
  codec_.SendMessage(conn_wp_.lock(), *message);
  pending_messages_.pop_front();

  if (message->type() != MT_MASTER) {
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
  codec_.SendMessage(conn_wp_.lock(), message);
}

}  // namespace saber
