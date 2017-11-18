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

void SaberSession::ReConnect(const voyager::TcpConnectionPtr& p) {
  MutexLock lock(&mutex_);
  closed_ = false;
  conn_wp_ = p;
  committer_->SetEventLoop(p->OwnerEventLoop());
  pending_messages_.clear();
}

bool SaberSession::OnMessage(std::unique_ptr<SaberMessage> message) {
  if (closed_) {
    return false;
  }
  // No need to check master when the pending_messages_ is not empty.
  if (message->type() == MT_PING && !pending_messages_.empty()) {
    return true;
  }

  if (message->type() == MT_CLOSE) {
    CloseRequest request;
    request.add_session_id(session_id_);
    request.add_version(version_);
    message->set_data(request.SerializeAsString());
  }

  bool next = false;
  {
    MutexLock lock(&mutex_);
    if (last_finished_) {
      next = true;
      last_finished_ = false;
    } else {
      if (message->type() == MT_MASTER) {
        pending_messages_.clear();
      }
      pending_messages_.push_back(std::move(message));
    }
  }
  if (next) {
    committer_->Commit(std::move(message));
  }
  return true;
}

void SaberSession::OnCommitComplete(std::unique_ptr<SaberMessage> message) {
  if (message->type() != MT_PING) {
    codec_.SendMessage(conn_wp_.lock(), *message);
  }

  SaberMessage* next = nullptr;
  {
    MutexLock lock(&mutex_);
    if (message->type() != MT_MASTER && message->type() != MT_CLOSE) {
      if (!pending_messages_.empty()) {
        next = pending_messages_.front().release();
        pending_messages_.pop_front();
      }
    } else {
      closed_ = true;
      voyager::TcpConnectionPtr p = conn_wp_.lock();
      if (p) {
        p->ShutDown();
      }
    }
    if (!next) {
      last_finished_ = true;
    }
  }
  if (next) {
    committer_->Commit(std::unique_ptr<SaberMessage>(next));
  }
}

void SaberSession::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  codec_.SendMessage(conn_wp_.lock(), message);
}

}  // namespace saber
