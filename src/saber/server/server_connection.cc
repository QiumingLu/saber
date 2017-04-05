// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(voyager::EventLoop* loop,
                                   std::unique_ptr<Messager> p,
                                   SaberDB* db,
                                   skywalker::Node* node)
    : closed_(false),
      last_finished_(true),
      loop_(loop),
      messager_(std::move(p)),
      committer_(new Committer(this, loop_, db, node)) {
  assert(messager_);
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
  committer_->SetCommitCompleteCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnCommitComplete(std::move(message));
  });
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  messager_->SendMessage(message);
}

void ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  assert(message);
  if (message->type() == MT_PING) {
  } else {
    pending_messages_.push(std::move(message));
    if (last_finished_) {
      last_finished_ = false;
      assert(!pending_messages_.empty());
      committer_->Commit(pending_messages_.front().get());
    }
  }
}

void ServerConnection::OnCommitComplete(
    std::unique_ptr<SaberMessage> message) {
  pending_messages_.pop();
  messager_->SendMessage(*(message.get()));
  if (message->type() != MT_MASTER && !closed_) {
    if (!pending_messages_.empty()) {
      assert(!last_finished_);
      committer_->Commit(pending_messages_.front().get());
    } else {
     last_finished_ = true;
    }
  } else {
    closed_ = true;
  }
}

}  // namespace saber
