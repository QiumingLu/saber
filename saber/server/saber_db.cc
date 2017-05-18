// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"
#include "saber/util/logging.h"

namespace saber {

SaberDB::SaberDB(uint32_t group_size) {
  for (uint32_t i = 0; i < group_size; ++i) {
    trees_.push_back(std::unique_ptr<DataTree>(new DataTree()));
  }
}

SaberDB::~SaberDB() {
}

void SaberDB::Create(uint32_t group_id, const CreateRequest& request,
                     const Transaction& txn, CreateResponse* response) {
  trees_[group_id]->Create(request, txn, response);
}

void SaberDB::Delete(uint32_t group_id, const DeleteRequest& request,
                     const Transaction& txn, DeleteResponse* response) {
  trees_[group_id]->Delete(request, txn, response);
}

void SaberDB::Exists(uint32_t group_id, const ExistsRequest& request,
                     Watcher* watcher, ExistsResponse* response) {
  trees_[group_id]->Exists(request, watcher, response);
}

void SaberDB::GetData(uint32_t group_id, const GetDataRequest& request,
                      Watcher* watcher, GetDataResponse* response) {
  trees_[group_id]->GetData(request, watcher, response);
}

void SaberDB::SetData(uint32_t group_id, const SetDataRequest& request,
                      const Transaction& txn, SetDataResponse* response) {
  trees_[group_id]->SetData(request, txn, response);
}

void SaberDB::GetACL(uint32_t group_id, const GetACLRequest& request,
                     GetACLResponse* response) {
  trees_[group_id]->GetACL(request, response);
}

void SaberDB::SetACL(uint32_t group_id, const SetACLRequest& request,
                     const Transaction& txn, SetACLResponse* response) {
  trees_[group_id]->SetACL(request, txn, response);
}

void SaberDB::GetChildren(uint32_t group_id,
                          const GetChildrenRequest& request,
                          Watcher* watcher, GetChildrenResponse* response) {
  trees_[group_id]->GetChildren(request, watcher, response);
}

void SaberDB::RemoveWatcher(Watcher* watcher) {
  for (auto& tree : trees_) {
    tree->RemoveWatcher(watcher);
  }
}

void SaberDB::KillSession(uint64_t session_id, const Transaction& txn) {
  for (auto& tree : trees_) {
    tree->KillSession(session_id, txn);
  }
}

bool SaberDB::Execute(uint32_t group_id,
                      uint64_t instance_id,
                      const std::string& value,
                      void* context) {
  SaberMessage message;
  message.ParseFromString(value);
  Transaction txn;
  txn.ParseFromString(message.extra_data());
  txn.set_group_id(group_id);
  txn.set_instance_id(instance_id);
  SaberMessage* reply_message = nullptr;
  if (context) {
    reply_message = reinterpret_cast<SaberMessage*>(context);
    assert(message.type() == reply_message->type());
  }
  switch (message.type()) {
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;  // FIXME response may be no neccessary here?
      request.ParseFromString(message.data());
      Create(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      DeleteResponse response;
      request.ParseFromString(message.data());
      Delete(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      SetDataResponse response;
      request.ParseFromString(message.data());
      SetData(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_SETACL: {
      SetACLRequest request;
      SetACLResponse response;
      request.ParseFromString(message.data());
      SetACL(group_id, request, txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  return true;
}

uint64_t SaberDB::GetCheckpointInstanceId(uint32_t group_id) {
  return -1;
}

bool SaberDB::LockCheckpoint(uint32_t group_id) {
  return true;
}

bool SaberDB::UnLockCheckpoint(uint32_t group_id) {
  return true;
}

bool SaberDB::GetCheckpoint(uint32_t group_id, int machine_id,
                            std::string* dir,
                            std::vector<std::string>* files) {
  return true;
}

bool SaberDB::LoadCheckpoint(uint32_t group_id, int machine_id,
                             const std::string& dir,
                             const std::vector<std::string>& files) {
  return true;
}

}  // namespace saber
