// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/committer.h"

#include <utility>

#include "saber/proto/server.pb.h"
#include "saber/server/saber_session.h"
#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

uint32_t Committer::kMaxDataSize = 1024 * 1024;

Committer::Committer(uint32_t group_id, SaberSession* session,
                     voyager::EventLoop* loop, SaberDB* db,
                     skywalker::Node* node)
    : group_id_(group_id),
      session_(session),
      loop_(loop),
      db_(db),
      node_(node) {}

void Committer::Commit(std::unique_ptr<SaberMessage> message) {
  if (message->type() != MT_MASTER && node_->IsMaster(group_id_)) {
    HandleCommit(std::move(message));
  } else {
    skywalker::Member i;
    uint64_t version;
    node_->GetMaster(group_id_, &i, &version);
    Master master;
    master.set_host(i.host);
    master.set_port(atoi(i.context.c_str()));
    message->set_type(MT_MASTER);
    message->set_data(master.SerializeAsString());
    session_->OnCommitComplete(std::move(message));
  }
}

void Committer::HandleCommit(std::unique_ptr<SaberMessage> message) {
  switch (message->type()) {
    case MT_PING: {
      break;
    }
    case MT_EXISTS: {
      ExistsRequest request;
      ExistsResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? session_ : nullptr;
      db_->Exists(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETDATA: {
      GetDataRequest request;
      GetDataResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? session_ : nullptr;
      db_->GetData(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETACL: {
      GetACLRequest request;
      GetACLResponse response;
      request.ParseFromString(message->data());
      db_->GetACL(group_id_, request, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenRequest request;
      GetChildrenResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? session_ : nullptr;
      db_->GetChildren(group_id_, request, watcher, &response);
      message->set_data(response.SerializeAsString());
      break;
    }
    // FIXME check version here may be more effective?
    case MT_SETDATA: {
      if (message->data().size() > kMaxDataSize) {
        SetDataResponse response;
        response.set_code(RC_FAILED);
        message->set_data(response.SerializeAsString());
        break;
      }
    }
    case MT_CREATE:
    case MT_DELETE:
    case MT_SETACL:
    case MT_CLOSE: {
      Propose(std::move(message));
      return;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  session_->OnCommitComplete(std::move(message));
}

void Committer::Propose(std::unique_ptr<SaberMessage> message) {
  Transaction txn;
  txn.set_session_id(session_->session_id());
  txn.set_time(NowMillis());
  SaberMessage* reply = message.release();
  reply->set_extra_data(txn.SerializeAsString());
  CommitterPtr ptr(shared_from_this());
  bool b = node_->Propose(
      group_id_, db_->machine_id(), reply->SerializeAsString(), reply,
      [ptr](uint64_t instance_id, const skywalker::Status& s, void* context) {
        if (!ptr.unique()) {
          ptr->OnProposeComplete(instance_id, s, context);
        }
      });
  if (!b) {
    SetFailedState(reply);
    reply->clear_extra_data();
    session_->OnCommitComplete(std::unique_ptr<SaberMessage>(reply));
  }
}

void Committer::OnProposeComplete(uint64_t instance_id,
                                  const skywalker::Status& s, void* context) {
  SaberMessage* reply_message = reinterpret_cast<SaberMessage*>(context);
  assert(reply_message);
  reply_message->clear_extra_data();
  if (!s.ok()) {
    SetFailedState(reply_message);
  }
  LOG_DEBUG("Committer::OnProposeComplete - %s.", s.ToString().c_str());

  CommitterPtr ptr(shared_from_this());
  loop_->QueueInLoop([ptr, reply_message]() {
    if (!ptr.unique()) {
      ptr->session_->OnCommitComplete(
          std::unique_ptr<SaberMessage>(reply_message));
    }
  });
}

void Committer::SetFailedState(SaberMessage* reply_message) {
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

}  // namespace saber
