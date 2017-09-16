// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

#include <stdlib.h>

#include <algorithm>

#include <skywalker/file.h>
#include <voyager/util/coding.h>

#include "saber/util/crc32c.h"
#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

namespace {
static const char* kCheckpoint = "CHECKPOINT-";
}

SaberDB::SaberDB(RunLoop* loop, const ServerOptions& options)
    : kKeepCheckpointCount(options.keep_checkpoint_count),
      kMakeCheckpointInterval(options.make_checkpoint_interval),
      lock_(false),
      doing_(false),
      checkpoint_storage_path_(options.checkpoint_storage_path),
      checkpoint_id_(options.paxos_group_size, UINTMAX_MAX),
      files_(options.paxos_group_size),
      generator_((unsigned)NowMillis()),
      distribution_(1, kMakeCheckpointInterval / 2),
      loop_(loop) {
  if (checkpoint_storage_path_[checkpoint_storage_path_.size() - 1] != '/') {
    checkpoint_storage_path_.push_back('/');
  }
  for (uint32_t i = 0; i < options.paxos_group_size; ++i) {
    trees_.push_back(std::unique_ptr<DataTree>(new DataTree()));
    sessions_.push_back(std::unique_ptr<SessionManager>(new SessionManager()));
  }
}

SaberDB::~SaberDB() {}

bool SaberDB::Recover() {
  skywalker::FileManager::Instance()->CreateDir(checkpoint_storage_path_);
  bool exists =
      skywalker::FileManager::Instance()->FileExists(checkpoint_storage_path_);
  if (!exists) {
    LOG_ERROR("checkpoint storage path(%s) access failed.",
              checkpoint_storage_path_.c_str());
    return false;
  }

  char* end;
  std::string s;
  std::vector<std::string> files;
  for (uint32_t i = 0; i < files_.size(); ++i) {
    std::string dir = checkpoint_storage_path_ + "g" + std::to_string(i);
    skywalker::FileManager::Instance()->CreateDir(dir);
    skywalker::FileManager::Instance()->GetChildren(dir, &files, true);
    for (auto& file : files) {
      size_t found = file.find_first_of("-");
      if (found != std::string::npos) {
        files_[i].push_back(strtoull(file.substr(found + 1).c_str(), &end, 10));
      }
    }
    std::sort(files_[i].begin(), files_[i].end());

    while (!files_[i].empty()) {
      std::string fname = FileName(i, files_[i].back());
      ReadFileToString(skywalker::FileManager::Instance(), fname, &s);
      if (Checksum(&s)) {
        uint64_t instance_id = voyager::DecodeFixed64(s.c_str());
        if (files_[i].back() != instance_id) {
          if (std::find(files_[i].begin(), files_[i].end(), instance_id) ==
              files_[i].end()) {
            files_[i].back() = instance_id;
            skywalker::FileManager::Instance()->RenameFile(
                fname, FileName(i, instance_id));
            std::sort(files_[i].begin(), files_[i].end());
            continue;
          }
        } else {
          checkpoint_id_[i] = instance_id;
          uint64_t index = trees_[i]->Recover(s, 8);
          index = sessions_[i]->Recover(s, index);
          assert(index == s.size());
          LOG_INFO("Group %u: recover from file %s", i, fname.c_str());
          break;
        }
      }
      DeleteFile(fname);
      files_[i].pop_back();
    }
  }
  return true;
}

bool SaberDB::Checksum(std::string* s) const {
  assert(s->size() > 4);
  if (s->size() > 4) {
    uint32_t c = voyager::DecodeFixed32(s->c_str() + s->size() - 4);
    if (c == crc::crc32(0, s->c_str(), s->size() - 4)) {
      s->erase(s->size() - 4);
      return true;
    }
  }
  return false;
}

std::string SaberDB::FileName(uint32_t group_id, uint64_t instance_id) const {
  return checkpoint_storage_path_ + "g" + std::to_string(group_id) + "/" +
         kCheckpoint + std::to_string(instance_id);
}

void SaberDB::DeleteFile(const std::string& fname) const {
  skywalker::FileManager::Instance()->DeleteFile(fname);
  LOG_INFO("Delete file %s.", fname.c_str());
}

void SaberDB::Create(uint32_t group_id, const CreateRequest& request,
                     const Transaction& txn, CreateResponse* response) const {
  trees_[group_id]->Create(request, txn, response);
}

void SaberDB::Delete(uint32_t group_id, const DeleteRequest& request,
                     const Transaction& txn, DeleteResponse* response) const {
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
                      const Transaction& txn, SetDataResponse* response) const {
  trees_[group_id]->SetData(request, txn, response);
}

void SaberDB::GetACL(uint32_t group_id, const GetACLRequest& request,
                     GetACLResponse* response) const {
  trees_[group_id]->GetACL(request, response);
}

void SaberDB::SetACL(uint32_t group_id, const SetACLRequest& request,
                     const Transaction& txn, SetACLResponse* response) const {
  trees_[group_id]->SetACL(request, txn, response);
}

void SaberDB::GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                          Watcher* watcher,
                          GetChildrenResponse* response) const {
  trees_[group_id]->GetChildren(request, watcher, response);
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

std::unordered_map<uint64_t, uint64_t>* SaberDB::CopySessions(
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
                          const Transaction& txn) const {
  trees_[group_id]->KillSession(session_id, txn);
}

bool SaberDB::Execute(uint32_t group_id, uint64_t instance_id,
                      const std::string& value, void* context) {
  bool result = true;
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
      request.ParseFromString(message.data());
      result = CreateSession(group_id, request.session_id(), instance_id,
                             request.version());
      break;
    }
    case MT_CLOSE: {
      CloseRequest request;
      request.ParseFromString(message.data());
      for (int i = 0; i < request.session_id_size(); ++i) {
        if (CloseSession(group_id, request.session_id(i), request.version(i))) {
          KillSession(group_id, request.session_id(i), txn);
        }
      }
      break;
    }
    case MT_CREATE: {
      CreateRequest request;
      CreateResponse response;
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
  MaybeMakeCheckpoint(group_id, instance_id);
  return result;
}

void SaberDB::MaybeMakeCheckpoint(uint32_t group_id, uint64_t instance_id) {
  bool expected = false;
  if (doing_.compare_exchange_strong(expected, true)) {
    uint64_t i = GetCheckpointInstanceId(group_id);
    uint32_t interval = kMakeCheckpointInterval / 2 + distribution_(generator_);
    if (((i == UINTMAX_MAX && instance_id > interval) ||
         (instance_id - i > interval)) &&
        LockCheckpoint(group_id)) {
      // FIXME when the data is too much, may be can use sync serialization.
      std::unordered_map<std::string, DataNode>* nodes =
          trees_[group_id]->CopyNodes();
      std::unordered_map<std::string, std::set<std::string>>* childrens =
          trees_[group_id]->CopyChildrens();
      std::unordered_map<uint64_t, uint64_t>* sessions =
          sessions_[group_id]->CopySessions();

      loop_->QueueInLoop(
          [this, group_id, instance_id, nodes, childrens, sessions]() {
            MakeCheckpoint(group_id, instance_id, nodes, childrens, sessions);
            UnLockCheckpoint(group_id);
            delete sessions;
            delete childrens;
            delete nodes;
            doing_ = false;
          });
    } else {
      doing_ = false;
    }
  }
}

void SaberDB::MakeCheckpoint(
    uint32_t group_id, uint64_t instance_id,
    std::unordered_map<std::string, DataNode>* nodes,
    std::unordered_map<std::string, std::set<std::string>>* childrens,
    std::unordered_map<uint64_t, uint64_t>* sessions) {
  std::string s;
  voyager::PutFixed64(&s, instance_id);
  DataTree::SerializeToString(*nodes, *childrens, &s,
                              12 + 8 + 16 * sessions->size());
  SessionManager::SerializeToString(*sessions, &s);
  voyager::PutFixed32(&s, crc::crc32(0, s.c_str(), s.size()));
  std::string fname = FileName(group_id, instance_id);
  skywalker::Status status = skywalker::WriteStringToFileSync(
      skywalker::FileManager::Instance(), s, fname);
  if (status.ok()) {
    checkpoint_id_[group_id] = instance_id;
    files_[group_id].push_back(instance_id);
    LOG_INFO("Group %u: make checkpoint successful, the file is %s.", group_id,
             fname.c_str());
    CleanCheckpoint(group_id);
  } else {
    LOG_INFO("Group %u: make checkpoint failed.", group_id);
    DeleteFile(fname);
  }
}

void SaberDB::CleanCheckpoint(uint32_t group_id) {
  while (files_[group_id].size() > kKeepCheckpointCount) {
    DeleteFile(FileName(group_id, files_[group_id].front()));
    files_[group_id].erase(files_[group_id].begin());
  }
}

uint64_t SaberDB::GetCheckpointInstanceId(uint32_t group_id) {
  // FIXME no thread safe
  return checkpoint_id_[group_id];
}

bool SaberDB::LockCheckpoint(uint32_t group_id) {
  bool expected = false;
  return lock_.compare_exchange_strong(expected, true);
}

bool SaberDB::UnLockCheckpoint(uint32_t group_id) {
  bool expected = true;
  return lock_.compare_exchange_strong(expected, false);
}

bool SaberDB::GetCheckpoint(uint32_t group_id, uint32_t machine_id,
                            std::string* dir, std::vector<std::string>* files) {
  assert(this->machine_id() == machine_id);
  *dir = checkpoint_storage_path_ + "g" + std::to_string(group_id);
  if (!(files_[group_id].empty())) {
    files->push_back(kCheckpoint + std::to_string(files_[group_id].back()));
  }
  return true;
}

bool SaberDB::LoadCheckpoint(uint32_t group_id, uint64_t instance_id,
                             uint32_t machine_id, const std::string& dir,
                             const std::vector<std::string>& files) {
  assert(this->machine_id() == machine_id);
  std::string d = dir;
  if (d[d.size() - 1] != '/') {
    d.push_back('/');
  }

  skywalker::Status status;

  std::string fname = FileName(group_id, instance_id);
  std::string s;
  for (auto& file : files) {
    status = skywalker::ReadFileToString(skywalker::FileManager::Instance(),
                                         d + file, &s);
    if (status.ok()) {
      status = skywalker::WriteStringToFileSync(
          skywalker::FileManager::Instance(), s, fname);
    }
    if (!status.ok()) {
      DeleteFile(fname);
      return false;
    }
  }

  checkpoint_id_[group_id] = instance_id;
  files_[group_id].push_back(instance_id);

  LOG_INFO("Group %u: load checkpoint successful! the file is %s.", group_id,
           fname.c_str());

  CleanCheckpoint(group_id);

  return true;
}

}  // namespace saber
