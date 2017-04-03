// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(std::unique_ptr<Messager> p,
                                   SaberDB* db,
                                   voyager::EventLoop* loop,
                                   skywalker::Node* node)
    : finished_(true),
      messager_(std::move(p)),
      loop_(loop),
      committer_(this, db, loop_, node) {
  assert(messager_);
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
  committer_.SetCommitCompleteCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnCommitComplete(std::move(message));
  });
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  loop_->QueueInLoop([this, event]() {
    SaberMessage message;
    message.set_type(MT_NOTIFICATION);
    message.set_data(event.SerializeAsString());
    messager_->SendMessage(message);
  });
}

void ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  if (message->type() == MT_PING) {
  } else {
    pending_messages_.push_back(std::move(message));
    if (finished_) {
      finished_ = false;
      committer_.Commit(*(pending_messages_.front().get()));
    }  
  }
}

void ServerConnection::OnCommitComplete(
    std::unique_ptr<SaberMessage> message) {
  pending_messages_.pop_front();
  messager_->SendMessage(*(message.get()));
  if (!pending_messages_.empty()) {
    assert(!finished_);
    committer_.Commit(*(pending_messages_.front().get()));
  } else {
   finished_ = true;
  }
}

}  // namespace saber
