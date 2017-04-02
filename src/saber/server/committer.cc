// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/committer.h"
#include "saber/server/server_connection.h"
#include "saber/util/logging.h"
#include "saber/util/murmurhash3.h"

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
}

Committer::~Committer() {
  delete context_;
}

void Committer::Commit(const SaberMessage& message) {
  uint32_t group_id = Shard(message.root_path());
  if (node_->IsMaster(group_id)) {
    Commit(message, group_id);
  } else {
    SaberMessage* reply_message = new SaberMessage();
    cb_(std::unique_ptr<SaberMessage>(reply_message));
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

void Committer::Commit(const SaberMessage& message, uint32_t group_id) {
  bool wait = false;
  SaberMessage* reply_message = new SaberMessage();
  switch (message.type()) {
    case MT_EXISTS: {
      ExistsRequest request;
      ExistsResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_EXISTS);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETDATA: {
      GetDataRequest request;
      GetDataResponse response;
      request.ParseFromString(message.data());
      Watcher* watcher = request.watch() ? conn_ : nullptr;
      db_->GetData(request.path(), watcher, &response);
      reply_message->set_type(MT_GETDATA);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETACL: {
      GetACLRequest request;
      GetACLResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_GETACL);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenRequest request;
      GetChildrenResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_GETCHILDREN);
      reply_message->set_data(response.SerializeAsString());
      break;
    }

    case MT_CREATE:
    case MT_DELETE: 
    case MT_SETDATA: 
    case MT_SETACL: {
      context_->user_data = reply_message;
      node_->Propose(
          group_id, message.SerializeAsString(), context_, 
          [this](skywalker::MachineContext* context, 
                 const skywalker::Status& s, uint64_t instance_id) {
        OnProposeComplete(context, s, instance_id);
      });
      wait = true;
      break; 
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
  if (!wait) {
    assert(cb_);
    cb_(std::unique_ptr<SaberMessage>(reply_message));
  }
}

bool Committer::Execute(uint32_t group_id,
                        uint64_t instance_id,
                        const std::string& value,
                        skywalker::MachineContext* context) {
  SaberMessage message;
  SaberMessage* reply_message = 
      reinterpret_cast<SaberMessage*>(context->user_data);
  message.ParseFromString(value);
  switch (message.type()) {
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_CREATE);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      DeleteResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_DELETE);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      SetDataResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_SETDATA);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    case MT_SETACL: {
      SetACLRequest request;
      SetACLResponse response;
      request.ParseFromString(message.data());
      reply_message->set_type(MT_SETACL);
      reply_message->set_data(response.SerializeAsString());
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
  return true; 
}

void Committer::OnProposeComplete(skywalker::MachineContext* context, 
                                  const skywalker::Status& s, 
                                  uint64_t instance_id) {
  SaberMessage* reply_message = 
      reinterpret_cast<SaberMessage*>(context->user_data);
  loop_->RunInLoop([this, reply_message]() {
    assert(cb_);
    cb_(std::unique_ptr<SaberMessage>(reply_message));
  });
}

}  // namespace saber
