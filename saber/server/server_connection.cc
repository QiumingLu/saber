// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(uint64_t session_id,
                                   const voyager::TcpConnectionPtr& p,
                                   SaberDB* db,
                                   skywalker::Node* node)
    : closed_(false),
      last_finished_(true),
      session_id_(session_id),
      conn_(p),
      messager_(new Messager()),
      committer_(new Committer(this, p->OwnerEventLoop(), db, node)) {
  messager_->SetTcpConnection(p);
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    return HandleMessage(std::move(message));
  });
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::OnMessage(
    const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
  messager_->OnMessage(p, buf);
}

bool ServerConnection::HandleMessage(std::unique_ptr<SaberMessage> message) {
  if (closed_) {
    return false;
  }
  if (message->type() == MT_PING) {
  } else {
    pending_messages_.push(std::move(message));
    if (last_finished_) {
      last_finished_ = false;
      committer_->Commit(pending_messages_.front().get());
    }
  }
  return true;
}

void ServerConnection::OnCommitComplete(
    std::unique_ptr<SaberMessage> message) {
  messager_->SendMessage(*message);
  assert(!pending_messages_.empty());
  pending_messages_.pop();
  if (message->type() != MT_MASTER) {
    if (!pending_messages_.empty() && !closed_) {
      assert(!last_finished_);
      committer_->Commit(pending_messages_.front().get());
    } else {
      last_finished_ = true;
    }
  } else {
    closed_ = true;
    conn_->ShutDown();
  }
}

void ServerConnection::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  messager_->SendMessage(message);
}

}  // namespace saber
