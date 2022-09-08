// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

#include <stdlib.h>

#include <algorithm>

#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

SaberDB::SaberDB(RunLoop* loop, const ServerOptions& options) {
  for (uint32_t i = 0; i < options.paxos_group_size; ++i) {
    trees_.push_back(std::unique_ptr<DataTree>(new DataTree()));
    sessions_.push_back(std::unique_ptr<SessionManager>(new SessionManager()));
  }
}

SaberDB::~SaberDB() {}

bool SaberDB::Recover(uint32_t group_id, uint64_t instance_id,
                      const std::string& dir) {
  DataNodeList node_list;
  if (!skywalker::StateMachine::ReadCheckpoint(
      group_id, instance_id, dir + "/" + kDataCheckpoint, &node_list)) {
    return false;
  }
  SessionList session_list;
  if (!skywalker::StateMachine::ReadCheckpoint(
      group_id, instance_id, dir + "/" + kSessionCheckpoint, &session_list)) {
    return false;
  }
  trees_[group_id]->Recover(node_list);
  sessions_[group_id]->Recover(session_list);
  LOG_INFO("Group %u - instance %llu saberdb recover success.",
           group_id, (unsigned long long)instance_id);
  return true;
}

bool SaberDB::MakeCheckpoint(uint32_t group_id, uint64_t instance_id,
                             const std::string& dir,
                             const FinishCheckpointCallback& cb) {
  DataNodeList node_list = trees_[group_id]->GetDataNodeList();
  SessionList session_list = sessions_[group_id]->GetSessionList();

  loop_->QueueInLoop([this, group_id, instance_id, dir, cb,
                      node_list = std::move(node_list),
                      session_list = std::move(session_list)]() {
    if (skywalker::StateMachine::WriteCheckpoint(
            group_id, instance_id, dir + "/" + kDataCheckpoint, node_list) &&
        skywalker::StateMachine::WriteCheckpoint(
            group_id, instance_id, dir + "/" + kSessionCheckpoint, session_list)) {
      cb(machine_id(), group_id, instance_id, true);
      LOG_INFO("Group %u - instance %llu saberdb make checkpoint success.",
              group_id, (unsigned long long)instance_id);
    } else {
      cb(machine_id(), group_id, instance_id, false);
    }
  });
  return true;
}

bool SaberDB::GetCheckpoint(uint32_t group_id, uint64_t instance_id,
                            const std::string& dir,
                            std::vector<std::string>* files) {
  files->push_back(kDataCheckpoint);
  files->push_back(kSessionCheckpoint);
  return true;
}

void SaberDB::Create(uint32_t group_id, const CreateRequest& request,
                     const Transaction* txn, CreateResponse* response) const {
  trees_[group_id]->Create(request, txn, response);
}

void SaberDB::Delete(uint32_t group_id, const DeleteRequest& request,
                     const Transaction* txn, DeleteResponse* response) const {
  trees_[group_id]->Delete(request, txn, response);
}

void SaberDB::Exists(uint32_t group_id, const ExistsRequest& request,
                     Watcher* watcher, ExistsResponse* response) const {
  trees_[group_id]->Exists(request, watcher, response);
}

void SaberDB::GetData(uint32_t group_id, const GetDataRequest& request,
                      Watcher* watcher, GetDataResponse* response) const {
  trees_[group_id]->GetData(request, watcher, response);
}

void SaberDB::SetData(uint32_t group_id, const SetDataRequest& request,
                      const Transaction* txn, SetDataResponse* response) const {
  trees_[group_id]->SetData(request, txn, response);
}

void SaberDB::GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                          Watcher* watcher,
                          GetChildrenResponse* response) const {
  trees_[group_id]->GetChildren(request, watcher, response);
}

void SaberDB::CheckCreate(uint32_t group_id, const CreateRequest& request,
                          CreateResponse* response) const {
  trees_[group_id]->Create(request, nullptr, response, true);
}

void SaberDB::CheckDelete(uint32_t group_id, const DeleteRequest& request,
                          DeleteResponse* response) const {
  trees_[group_id]->Delete(request, nullptr, response, true);
}

void SaberDB::CheckSetData(uint32_t group_id, const SetDataRequest& request,
                           SetDataResponse* response) const {
  trees_[group_id]->SetData(request, nullptr, response, true);
}

void SaberDB::RemoveWatcher(uint32_t group_id, Watcher* watcher) const {
  trees_[group_id]->RemoveWatcher(watcher);
}

bool SaberDB::FindSession(uint32_t group_id, uint64_t session_id,
                          uint64_t* version) const {
  return sessions_[group_id]->FindSession(session_id, version);
}

bool SaberDB::FindSession(uint32_t group_id, uint64_t session_id,
                          uint64_t version) const {
  return sessions_[group_id]->FindSession(session_id, version);
}

std::unordered_map<uint64_t, uint64_t> SaberDB::CopySessions(
    uint32_t group_id) const {
  return sessions_[group_id]->CopySessions();
}

bool SaberDB::CreateSession(uint32_t group_id, uint64_t session_id,
                            uint64_t new_version, uint64_t old_version) const {
  return sessions_[group_id]->CreateSession(session_id, new_version,
                                            old_version);
}

bool SaberDB::CloseSession(uint32_t group_id, uint64_t session_id,
                           uint64_t version) const {
  return sessions_[group_id]->CloseSession(session_id, version);
}

void SaberDB::KillSession(uint32_t group_id, uint64_t session_id,
                          const Transaction* txn) const {
  trees_[group_id]->KillSession(session_id, txn);
}

bool SaberDB::Execute(uint32_t group_id, uint64_t instance_id,
                      const std::string& value, void* context) {
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
    case MT_CONNECT: {
      ConnectRequest request;
      ConnectResponse response;
      request.ParseFromString(message.data());
      if (!CreateSession(group_id, request.session_id(), instance_id,
                         request.version())) {
        response.set_code(RC_UNKNOWN);
      }
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_CLOSE: {
      CloseRequest request;
      request.ParseFromString(message.data());
      for (int i = 0; i < request.session_id_size(); ++i) {
        if (CloseSession(group_id, request.session_id(i), request.version(i))) {
          KillSession(group_id, request.session_id(i), &txn);
        }
      }
      break;
    }
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;
      request.ParseFromString(message.data());
      Create(group_id, request, &txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      DeleteResponse response;
      request.ParseFromString(message.data());
      Delete(group_id, request, &txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      SetDataResponse response;
      request.ParseFromString(message.data());
      SetData(group_id, request, &txn, &response);
      if (reply_message) {
        reply_message->set_data(response.SerializeAsString());
      }
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.");
      return false;
      break;
    }
  }
  return true;
}

}  // namespace saber
