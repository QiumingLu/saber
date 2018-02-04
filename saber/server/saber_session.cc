// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_session.h"

#include <voyager/core/eventloop.h>

#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"
#include "saber/util/timeops.h"

namespace saber {

uint32_t SaberSession::kMaxDataSize = 1024 * 1024;

static std::string GetRoot(const std::string& path) {
  size_t i = 0;
  for (i = 1; i < path.size(); ++i) {
    if (path[i] == '/') {
      break;
    }
  }
  assert(i > 1);
  return path.substr(0, i);
}

SaberSession::SaberSession(const std::string& root, uint32_t group_id,
                           uint64_t session_id,
                           const voyager::TcpConnectionPtr& p, SaberDB* db,
                           skywalker::Node* node)
    : kRoot(root),
      group_id_(group_id),
      session_id_(session_id),
      version_(0),
      closed_(false),
      last_finished_(true),
      conn_wp_(p),
      db_(db),
      node_(node) {}

SaberSession::~SaberSession() { db_->RemoveWatcher(group_id_, this); }

void SaberSession::OnConnect(const voyager::TcpConnectionPtr& p) {
  MutexLock lock(&mutex_);
  closed_ = false;
  conn_wp_ = p;
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
    HandleMessage(std::move(message));
  }
  return true;
}

void SaberSession::HandleMessage(std::unique_ptr<SaberMessage> message) {
  if (message->type() != MT_MASTER && node_->IsMaster(group_id_)) {
    DoIt(std::move(message));
  } else {
    skywalker::Member i;
    uint64_t version;
    node_->GetMaster(group_id_, &i, &version);
    Master master;
    master.set_host(i.host);
    master.set_port(atoi(i.context.c_str()));
    message->set_type(MT_MASTER);
    message->set_data(master.SerializeAsString());
    Done(std::move(message));
  }
}

void SaberSession::DoIt(std::unique_ptr<SaberMessage> message) {
  bool done = true;
  switch (message->type()) {
    case MT_PING: {
      break;
    }
    case MT_EXISTS: {
      ExistsRequest request;
      ExistsResponse response;
      request.ParseFromString(message->data());
      assert(GetRoot(request.path()) == kRoot);
      Watcher* watcher = request.watch() ? this : nullptr;
      db_->Exists(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETDATA: {
      GetDataRequest request;
      GetDataResponse response;
      request.ParseFromString(message->data());
      assert(GetRoot(request.path()) == kRoot);
      Watcher* watcher = request.watch() ? this : nullptr;
      db_->GetData(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETACL: {
      GetACLRequest request;
      GetACLResponse response;
      request.ParseFromString(message->data());
      assert(GetRoot(request.path()) == kRoot);
      db_->GetACL(group_id_, request, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenRequest request;
      GetChildrenResponse response;
      request.ParseFromString(message->data());
      assert(GetRoot(request.path()) == kRoot);
      Watcher* watcher = request.watch() ? this : nullptr;
      db_->GetChildren(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;
      request.ParseFromString(message->data());
      if (GetRoot(request.path()) != kRoot) {
        SetFailedState(message.get());
        break;
      }
      db_->CheckCreate(group_id_, request, &response);
      if (response.code() != RC_OK) {
        message->set_data(response.SerializeAsString());
      } else {
        done = false;
      }
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      DeleteResponse response;
      request.ParseFromString(message->data());
      if (GetRoot(request.path()) != kRoot) {
        SetFailedState(message.get());
        break;
      }
      db_->CheckDelete(group_id_, request, &response);
      if (response.code() != RC_OK) {
        message->set_data(response.SerializeAsString());
      } else {
        done = false;
      }
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      SetDataResponse response;
      request.ParseFromString(message->data());
      if (GetRoot(request.path()) != kRoot ||
          request.data().size() > kMaxDataSize) {
        SetFailedState(message.get());
        break;
      }
      db_->CheckSetData(group_id_, request, &response);
      if (response.code() != RC_OK) {
        message->set_data(response.SerializeAsString());
      } else {
        done = false;
      }
      break;
    }
    case MT_SETACL: {
      SetACLRequest request;
      SetACLResponse response;
      request.ParseFromString(message->data());
      if (GetRoot(request.path()) != kRoot) {
        SetFailedState(message.get());
        break;
      }
      db_->CheckSetACL(group_id_, request, &response);
      if (response.code() != RC_OK) {
        message->set_data(response.SerializeAsString());
      } else {
        done = false;
      }
      break;
    }
    case MT_CLOSE: {
      done = false;
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  if (done) {
    Done(std::move(message));
  } else {
    Propose(std::move(message));
  }
}

void SaberSession::Done(std::unique_ptr<SaberMessage> reply_message) {
  voyager::TcpConnectionPtr p = conn_wp_.lock();
  if (reply_message->type() != MT_PING) {
    codec_.SendMessage(p, *reply_message);
  }

  SaberMessage* next = nullptr;
  {
    MutexLock lock(&mutex_);
    if (p && reply_message->type() != MT_MASTER &&
        reply_message->type() != MT_CLOSE) {
      if (!pending_messages_.empty()) {
        next = pending_messages_.front().release();
        pending_messages_.pop_front();
      }
    } else {
      closed_ = true;
      pending_messages_.clear();
      if (p) {
        p->ForceClose();
      }
    }
    if (!next) {
      last_finished_ = true;
    }
  }
  if (next) {
    // FIXME
    p->OwnerEventLoop()->QueueInLoop(
        [this, next]() { HandleMessage(std::unique_ptr<SaberMessage>(next)); });
  }
}

void SaberSession::Propose(std::unique_ptr<SaberMessage> message) {
  Transaction txn;
  txn.set_session_id(session_id_);
  txn.set_time(NowMillis());
  SaberMessage* reply = message.release();
  reply->set_extra_data(txn.SerializeAsString());
  bool b = node_->Propose(
      group_id_, db_->machine_id(), reply->SerializeAsString(), reply,
      std::bind(&SaberSession::WeakCallback,
                std::weak_ptr<SaberSession>(shared_from_this()),
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3));
  if (!b) {
    SetFailedState(reply);
    reply->clear_extra_data();
    Done(std::unique_ptr<SaberMessage>(reply));
  }
}

void SaberSession::WeakCallback(std::weak_ptr<SaberSession> session_wp,
                                uint64_t instance_id,
                                const skywalker::Status& s, void* context) {
  SaberMessage* reply_message = reinterpret_cast<SaberMessage*>(context);
  assert(reply_message);
  std::shared_ptr<SaberSession> session(session_wp.lock());
  if (session) {
    if (!s.ok()) {
      SetFailedState(reply_message);
    }
    reply_message->clear_extra_data();
    LOG_DEBUG("Group %u: session(id=%llu) propose:%s", session->group_id_,
              (unsigned long long)session->session_id_, s.ToString().c_str());
    session->Done(std::unique_ptr<SaberMessage>(reply_message));
  } else {
    delete reply_message;
  }
}

void SaberSession::SetFailedState(SaberMessage* reply_message) {
  switch (reply_message->type()) {
    case MT_CREATE: {
      CreateResponse response;
      response.set_code(RC_FAILED);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_DELETE: {
      DeleteResponse response;
      response.set_code(RC_FAILED);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_SETDATA: {
      SetDataResponse response;
      response.set_code(RC_FAILED);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_SETACL: {
      SetACLResponse response;
      response.set_code(RC_FAILED);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_CLOSE: {
      CloseResponse response;
      response.set_code(RC_FAILED);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
}

void SaberSession::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  codec_.SendMessage(conn_wp_.lock(), message);
}

}  // namespace saber
