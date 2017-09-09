// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_session.h"

#include <utility>

namespace saber {

SaberSession::SaberSession(uint32_t group_id, uint64_t session_id,
                           const voyager::TcpConnectionPtr& p, SaberDB* db,
                           skywalker::Node* node)
    : closed_(false),
      last_finished_(true),
      group_id_(group_id),
      session_id_(session_id),
      conn_wp_(p),
      db_(db),
      committer_(new Committer(group_id, this, p->OwnerEventLoop(), db, node)) {
}

SaberSession::~SaberSession() { db_->RemoveWatcher(group_id_, this); }

void SaberSession::Connect(const voyager::TcpConnectionPtr& p) {
  closed_ = false;
  pending_messages_.clear();
  conn_wp_ = p;
  committer_->SetEventLoop(p->OwnerEventLoop());
}

void SaberSession::ShutDown() {
  closed_ = true;
  pending_messages_.clear();
  voyager::TcpConnectionPtr p = conn_wp_.lock();
  if (p) {
    p->ShutDown();
  }
}

bool SaberSession::OnMessage(std::unique_ptr<SaberMessage> message) {
  if (closed_) {
    return false;
  }
  // No need to check master when the pending_messages_ is not empty.
  if (message->type() == MT_PING && !pending_messages_.empty()) {
    return true;
  }

  if (message->type() == MT_MASTER) {
    pending_messages_.clear();
  }
  if (message->type() == MT_CLOSE) {
    CloseRequest request;
    request.add_session_id(session_id_);
    request.add_version(version_);
    message->set_data(request.SerializeAsString());
  }

  pending_messages_.push_back(std::move(message));
  if (last_finished_) {
    last_finished_ = false;
    // FIXME check the session has been moved?
    committer_->Commit(pending_messages_.front().get());
  }
  return true;
}

void SaberSession::OnCommitComplete(std::unique_ptr<SaberMessage> message) {
  assert(!pending_messages_.empty());
  if (message->type() != MT_PING) {
    message->set_id(pending_messages_.front()->id());
    codec_.SendMessage(conn_wp_.lock(), *message);
  }
  pending_messages_.pop_front();

  if (message->type() != MT_MASTER && message->type() != MT_CLOSE) {
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

void SaberSession::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  codec_.SendMessage(conn_wp_.lock(), message);
}

}  // namespace saber
