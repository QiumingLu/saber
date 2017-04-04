// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/committer.h"
#include "saber/server/server_connection.h"
#include "saber/util/logging.h"
#include "saber/util/murmurhash3.h"
#include "saber/util/timeops.h"
#include "saber/proto/server.pb.h"

namespace saber {

Committer::Committer(ServerConnection* conn,
                     SaberDB* db,
                     voyager::EventLoop* loop,
                     skywalker::Node* node)
    : conn_(conn),
      db_(db),
      loop_(loop),
      node_(node),
      context_(new skywalker::MachineContext(GetMachineId())) {
  node_->AddMachine(this);
}

Committer::~Committer() {
  node_->RemoveMachine(this);
  delete context_;
}

void Committer::Commit(SaberMessage* message) {
  uint32_t group_id = Shard(message->extra_data());
  if (node_->IsMaster(group_id)) {
    Commit(group_id, message);
  } else {
    assert(cb_);
    skywalker::IpPort i;
    uint64_t version;
    node_->GetMaster(group_id, &i, &version);
    Master master;
    master.set_ip(i.ip);
    master.set_port(static_cast<int>(i.port));
    SaberMessage* reply_message = new SaberMessage();
    reply_message->set_type(MT_MASTER);
    reply_message->set_data(master.SerializeAsString());
    cb_(std::unique_ptr<SaberMessage>(reply_message));
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
      db_->GetChildren(group_id, request, watcher, & response);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_CREATE:
    case MT_DELETE:
    case MT_SETDATA:
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
    assert(cb_);
    cb_(std::unique_ptr<SaberMessage>(reply_message));
  }
}

bool Committer::Propose(uint32_t group_id, 
                        SaberMessage* message, SaberMessage* reply_message) {
  uint64_t micros = NowMicros();
  char extra[64];
  snprintf(extra, sizeof(extra), "%" PRIu64"", micros);
  message->set_extra_data(extra);
  context_->user_data = reply_message;
  bool res = node_->Propose(
      group_id, message->SerializeAsString(), context_,
      [this](skywalker::MachineContext* context,
             const skywalker::Status& s, uint64_t instance_id) {
    OnProposeComplete(context, s, instance_id);
  });
  if (!res) {
    SetFailedState(reply_message);
  }
  return res; 
}

bool Committer::Execute(uint32_t group_id,
                        uint64_t instance_id,
                        const std::string& value,
                        skywalker::MachineContext* context) {
  SaberMessage message;
  message.ParseFromString(value);
  uint64_t time;
  memcpy(&time, message.extra_data().data(), sizeof(time));
  Transaction txn;
  txn.set_group_id(group_id);
  txn.set_instance_id(instance_id);
  txn.set_time(time);
  SaberMessage* reply_message = nullptr;
  if (context) {
    reply_message = reinterpret_cast<SaberMessage*>(context->user_data);
    assert(GetMachineId() == context->machine_id);
    assert(message.type() == reply_message->type());
  }
  switch (message.type()) {
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;
      request.ParseFromString(message.data());
      db_->Create(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      DeleteResponse response;
      request.ParseFromString(message.data());
      db_->Delete(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      SetDataResponse response;
      request.ParseFromString(message.data());
      db_->SetData(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_SETACL: {
      SetACLRequest request;
      SetACLResponse response;
      request.ParseFromString(message.data());
      db_->SetACL(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
  return true;
}

void Committer::OnProposeComplete(skywalker::MachineContext* context,
                                  const skywalker::Status& s,
                                  uint64_t instance_id) {
  assert(context);
  assert(GetMachineId() == context->machine_id);
  SaberMessage* reply_message =
      reinterpret_cast<SaberMessage*>(context->user_data);
  assert(reply_message);
  if (!s.ok()) {
    SetFailedState(reply_message);
  } else {
    LOG_INFO("Committer::OnProposeComplete - %s\n", s.ToString().c_str());
  }
  loop_->QueueInLoop([this, reply_message]() {
    assert(cb_);
    cb_(std::unique_ptr<SaberMessage>(reply_message));
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
