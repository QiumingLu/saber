// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/committer.h"
#include "saber/server/server_connection.h"
#include "saber/server/saber_cell.h"
#include "saber/util/logging.h"
#include "saber/util/murmurhash3.h"
#include "saber/util/timeops.h"
#include "saber/proto/server.pb.h"

namespace saber {

Committer::Committer(ServerConnection* conn,
                     voyager::EventLoop* loop,
                     SaberDB* db,
                     skywalker::Node* node)
    : conn_(conn),
      loop_(loop),
      db_(db),
      node_(node),
      context_(new skywalker::MachineContext(db->machine_id())) {
}

void Committer::Commit(SaberMessage* message) {
  uint32_t group_id = Shard(message->extra_data());
  if (node_->IsMaster(group_id)) {
    Commit(group_id, message);
  } else {
    skywalker::IpPort i;
    uint64_t version;
    node_->GetMaster(group_id, &i, &version);
    ServerMessage master_message;
    SaberCell::Instance()->FindServerByPaxosIpPort(
        std::make_pair(i.ip, i.port), &master_message);
    Master master;
    master.set_ip(master_message.server_ip);
    master.set_port(static_cast<int>(master_message.client_port));
    SaberMessage* reply_message = new SaberMessage();
    reply_message->set_type(MT_MASTER);
    reply_message->set_data(master.SerializeAsString());
    if (cb_) {
      cb_(std::unique_ptr<SaberMessage>(reply_message));
    }
  }
}

void Committer::Commit(uint32_t group_id, SaberMessage* message) {
  bool wait = false;
  SaberMessage* reply_message = new SaberMessage();
  reply_message->set_type(message->type());

  switch (message->type()) {
    case MT_EXISTS: {
      ExistsRequest request;
      ExistsResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? conn_ : nullptr;
      db_->Exists(group_id, request, watcher, &response);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETDATA: {
      GetDataRequest request;
      GetDataResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? conn_ : nullptr;
      db_->GetData(group_id, request, watcher, &response);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETACL: {
      GetACLRequest request;
      GetACLResponse response;
      request.ParseFromString(message->data());
      db_->GetACL(group_id, request, &response);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenRequest request;
      GetChildrenResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? conn_ : nullptr;
      db_->GetChildren(group_id, request, watcher, &response);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_CREATE:
    case MT_DELETE:
    case MT_SETDATA: // FIXME check version here may be more effective?
    case MT_SETACL: {
      wait = Propose(group_id, message, reply_message);
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
  if (!wait) {
    if (cb_) {
      cb_(std::unique_ptr<SaberMessage>(reply_message));
    }
  }
}

bool Committer::Propose(uint32_t group_id,
                        SaberMessage* message, SaberMessage* reply_message) {
  Transaction txn;
  txn.set_session_id(conn_->session_id());
  txn.set_time(NowMicros());
  message->set_extra_data(txn.SerializeAsString());
  assert(context_->user_data == nullptr);
  context_->user_data = reply_message;

  CommitterPtr ptr(shared_from_this());
  bool res = node_->Propose(
      group_id, message->SerializeAsString(), context_.get(),
      [ptr](skywalker::MachineContext* context,
            const skywalker::Status& s, uint64_t instance_id) {
    if (!ptr.unique()) {
      ptr->OnProposeComplete(context, s, instance_id);
    }
  });
  if (!res) {
    SetFailedState(reply_message);
  }
  return res;
}

void Committer::OnProposeComplete(skywalker::MachineContext* context,
                                  const skywalker::Status& s,
                                  uint64_t instance_id) {
  assert(context_.get() == context);
  SaberMessage* reply_message =
      reinterpret_cast<SaberMessage*>(context->user_data);
  assert(reply_message);
  context->user_data = nullptr;
  if (!s.ok()) {
    SetFailedState(reply_message);
  }
  LOG_DEBUG("Committer::OnProposeComplete - %s\n", s.ToString().c_str());

  CommitterPtr ptr(shared_from_this());
  loop_->QueueInLoop([ptr, reply_message]() {
    if (!ptr.unique()) {
      if (ptr->cb_) {
        ptr->cb_(std::unique_ptr<SaberMessage>(reply_message));
      }
    }
  });
}

void Committer::SetFailedState(SaberMessage* reply_message) {
  switch(reply_message->type()) {
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
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
}

uint32_t Committer::Shard(const std::string& s) {
  if (node_->group_size() == 1) {
    return 0;
  } else {
    uint32_t h;
    MurmurHash3_x86_32(s.c_str(), static_cast<int>(s.size()), 0, &h);
    return (h % node_->group_size());
  }
}

}  // namespace saber
