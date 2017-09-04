// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

#include <stdlib.h>

#include <algorithm>

#include <skywalker/file.h>
#include <voyager/util/coding.h>
#include <voyager/util/string_util.h>

#include "saber/util/crc32c.h"
#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"
#include "saber/util/timeops.h"

namespace saber {

SaberDB::SaberDB(const ServerOptions& options)
    : kKeepCheckpointCount(options.keep_checkpoint_count),
      kMakeCheckpointInterval(options.make_checkpoint_interval),
      lock_(false),
      doing_(false),
      checkpoint_storage_path_(options.checkpoint_storage_path),
      file_map_(options.paxos_group_size),
      sessions_(options.paxos_group_size),
      distribution_(1, kMakeCheckpointInterval / 2) {
  if (checkpoint_storage_path_[checkpoint_storage_path_.size() - 1] != '/') {
    checkpoint_storage_path_.push_back('/');
  }
  for (uint32_t i = 0; i < options.paxos_group_size; ++i) {
    trees_.push_back(std::unique_ptr<DataTree>(new DataTree()));
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
  std::set<uint64_t> temp;
  for (uint32_t i = 0; i < trees_.size(); ++i) {
    std::string dir = checkpoint_storage_path_ + "g" + std::to_string(i);
    skywalker::FileManager::Instance()->CreateDir(dir);
    skywalker::FileManager::Instance()->GetChildren(dir, &files, true);

    for (auto& file : files) {
      if (file == "g") {
        if (!GroupFile(i)) {
          std::string backup = dir + "/backup";
          if (skywalker::FileManager::Instance()->FileExists(backup)) {
            skywalker::FileManager::Instance()->RenameFile(backup, dir + "/g");
            GroupFile(i);
          }
        }
      } else if (file != "backup") {
        temp.insert(strtoull(file.c_str(), &end, 10));
      }
    }

    CheckFiles(i, temp);

    while (!temp.empty()) {
      std::string fname = dir + "/" + std::to_string(*(temp.rbegin()));
      ReadFileToString(skywalker::FileManager::Instance(), fname, &s);
      if (CheckData(&s)) {
        uint64_t index = trees_[i]->Recover(s);
        ParseFromArray(i, s.c_str() + index, s.size() - index);
        LOG_INFO("Group %u: recover from checkpoint file %s", i, fname.c_str());
        break;
      }
      DeleteFile(i, *(temp.rbegin()));
      file_map_[i].erase(std::prev(file_map_[i].end()));
      temp.erase(std::prev(temp.end()));
    }
    CleanCheckpoint(i);
    s.clear();
    files.clear();
    temp.clear();
  }
  loop_ = thread_.Loop();
  return true;
}

bool SaberDB::GroupFile(uint32_t group_id) {
  std::string fname =
      checkpoint_storage_path_ + "g" + std::to_string(group_id) + "/g";
  std::string s;
  skywalker::ReadFileToString(skywalker::FileManager::Instance(), fname, &s);
  if (CheckData(&s)) {
    char* end;
    std::vector<std::string> result;
    voyager::SplitStringUsing(s, "\r\n", &result);
    for (auto& one : result) {
      size_t found = one.find_first_of(":");
      uint64_t f = strtoull(one.substr(0, found).c_str(), &end, 10);
      uint64_t id = strtoull(one.substr(found + 1).c_str(), &end, 10);
      file_map_[group_id].insert(std::make_pair(f, id));
    }
    return true;
  } else {
    skywalker::FileManager::Instance()->DeleteFile(fname);
    return false;
  }
}

bool SaberDB::CheckData(std::string* s) {
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

void SaberDB::DeleteFile(uint32_t group_id, uint64_t instance_id) {
  std::string fname = checkpoint_storage_path_ + "g" +
                      std::to_string(group_id) + "/" +
                      std::to_string(instance_id);
  skywalker::FileManager::Instance()->DeleteFile(fname);
  LOG_INFO("Delete file %s.", fname.c_str());
}

void SaberDB::CheckFiles(uint32_t group_id, std::set<uint64_t>& temp) {
  for (std::map<uint64_t, uint64_t>::iterator it = file_map_[group_id].begin();
       it != file_map_[group_id].end();) {
    if (temp.find(it->first) == temp.end()) {
      it = file_map_[group_id].erase(it);
    } else {
      ++it;
    }
  }

  for (std::set<uint64_t>::iterator it = temp.begin(); it != temp.end();) {
    if (file_map_[group_id].find(*it) == file_map_[group_id].end()) {
      DeleteFile(group_id, *it);
      it = temp.erase(it);
    } else {
      ++it;
    }
  }
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

void SaberDB::GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                          Watcher* watcher, GetChildrenResponse* response) {
  trees_[group_id]->GetChildren(request, watcher, response);
}

void SaberDB::RemoveWatcher(uint32_t group_id, Watcher* watcher) {
  trees_[group_id]->RemoveWatcher(watcher);
}

bool SaberDB::FindSession(uint64_t group_id, uint64_t session_id,
                          uint64_t* version) const {
  MutexLock lock(&mutex_);
  auto it = sessions_[group_id].find(session_id);
  if (it != sessions_[group_id].end()) {
    *version = it->second;
    return true;
  }
  return false;
}

bool SaberDB::FindSession(uint64_t group_id, uint64_t session_id,
                          uint64_t version) const {
  MutexLock lock(&mutex_);
  auto it = sessions_[group_id].find(session_id);
  if (it != sessions_[group_id].end()) {
    return it->second == version;
  }
  return false;
}

bool SaberDB::CreateSession(uint32_t group_id, uint64_t session_id,
                            uint64_t new_version, uint64_t old_version) {
  MutexLock lock(&mutex_);
  if (old_version != 0) {
    auto it = sessions_[group_id].find(session_id);
    if (it != sessions_[group_id].end() && it->second == old_version) {
      it->second = new_version;
      return true;
    } else {
      return false;
    }
  } else {
    sessions_[group_id][session_id] = new_version;
    return true;
  }
}

bool SaberDB::CloseSession(uint32_t group_id, uint64_t session_id,
                           uint64_t version) {
  MutexLock lock(&mutex_);
  auto it = sessions_[group_id].find(session_id);
  if (it != sessions_[group_id].end()) {
    if (it->second == version) {
      sessions_[group_id].erase(it);
      return true;
    }
    return false;
  }
  return true;
}

void SaberDB::KillSession(uint32_t group_id, uint64_t session_id,
                          const Transaction& txn) {
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
      request.ParseFromString(message.data());
      return CreateSession(group_id, request.session_id(), instance_id,
                           txn.time());
    }
    case MT_CLOSE: {
      CloseRequest request;
      request.ParseFromString(message.data());
      if (CloseSession(group_id, request.session_id(), txn.time())) {
        KillSession(group_id, request.session_id(), txn);
      }
      break;
    }
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
  MaybeMakeCheckpoint(group_id, instance_id);
  return true;
}

void SaberDB::MaybeMakeCheckpoint(uint32_t group_id, uint64_t instance_id) {
  bool expected = false;
  if (doing_.compare_exchange_strong(expected, true)) {
    uint64_t i = file_map_[group_id].empty()
                     ? UINTMAX_MAX
                     : file_map_[group_id].rbegin()->second;
    uint32_t interval = kMakeCheckpointInterval / 2 + distribution_(generator_);
    if (((i == UINTMAX_MAX && instance_id > interval) ||
         (instance_id - i > interval)) &&
        LockCheckpoint(group_id)) {
      // FIXME
      std::unordered_map<uint64_t, uint64_t>* sessions =
          new std::unordered_map<uint64_t, uint64_t>(sessions_[group_id]);
      std::string* s = new std::string();
      trees_[group_id]->SerializeToString(s, 4 + 8 + 16 * sessions->size());
      loop_->QueueInLoop([this, group_id, instance_id, sessions, s]() {
        SerializeToString(*sessions, s);
        MakeCheckpoint(group_id, instance_id, s);
        delete sessions;
        delete s;
        UnLockCheckpoint(group_id);
        doing_ = false;
      });
    } else {
      doing_ = false;
    }
  }
}

void SaberDB::MakeCheckpoint(uint32_t group_id, uint64_t instance_id,
                             std::string* s) {
  uint64_t f = NowMillis();
  voyager::PutFixed32(s, crc::crc32(0, s->c_str(), s->size()));
  std::string fname = checkpoint_storage_path_ + "g" +
                      std::to_string(group_id) + "/" + std::to_string(f);
  skywalker::Status status = skywalker::WriteStringToFileSync(
      skywalker::FileManager::Instance(), *s, fname);
  if (status.ok()) {
    file_map_[group_id].insert(std::make_pair(f, instance_id));
    LOG_INFO(
        "Group %u: make checkpoint successful, the file is %s, instance_id "
        "is %llu",
        group_id, fname.c_str(), (unsigned long long)instance_id);
    CleanCheckpoint(group_id);
  } else {
    LOG_INFO("Group %u: make checkpoint failed.", group_id);
    skywalker::FileManager::Instance()->DeleteFile(fname);
  }
}

void SaberDB::CleanCheckpoint(uint32_t group_id) {
  while (file_map_[group_id].size() > kKeepCheckpointCount) {
    DeleteFile(group_id, file_map_[group_id].begin()->first);
    file_map_[group_id].erase(file_map_[group_id].begin());
  }
  std::string s;
  for (auto& file : file_map_[group_id]) {
    s.append(std::to_string(file.first));
    s.append(":");
    s.append(std::to_string(file.second));
    s.append("\r\n");
  }
  if (!s.empty()) {
    voyager::PutFixed32(&s, crc::crc32(0, s.c_str(), s.size()));
    std::string dir =
        checkpoint_storage_path_ + "g" + std::to_string(group_id) + "/";
    std::string fname = dir + "g";
    if (skywalker::FileManager::Instance()->FileExists(fname)) {
      skywalker::FileManager::Instance()->RenameFile(fname, dir + "backup");
    }
    skywalker::WriteStringToFileSync(skywalker::FileManager::Instance(), s,
                                     fname);
  }
}

uint64_t SaberDB::GetCheckpointInstanceId(uint32_t group_id) {
  return file_map_[group_id].empty() ? UINTMAX_MAX
                                     : file_map_[group_id].rbegin()->second;
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
  if (!(file_map_[group_id].empty())) {
    files->push_back(std::to_string(file_map_[group_id].rbegin()->first));
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

  uint64_t f = NowMillis();
  std::string fname = checkpoint_storage_path_ + "g" +
                      std::to_string(group_id) + "/" + std::to_string(f);
  std::string s;

  for (auto& file : files) {
    status = skywalker::ReadFileToString(skywalker::FileManager::Instance(),
                                         d + file, &s);
    if (status.ok()) {
      status = skywalker::WriteStringToFileSync(
          skywalker::FileManager::Instance(), s, fname);
    }
    if (!status.ok()) {
      skywalker::FileManager::Instance()->DeleteFile(fname);
      return false;
    }
  }

  file_map_[group_id].insert(std::make_pair(f, instance_id));

  LOG_INFO(
      "Group %u: load checkpoint successful! the file is %s, instance_id is "
      "%llu",
      group_id, fname.c_str(), (unsigned long long)instance_id);

  CleanCheckpoint(group_id);

  return true;
}

void SaberDB::ParseFromArray(uint32_t group_id, const char* s, size_t len) {
  size_t size = voyager::DecodeFixed64(s);
  s += 8;
  size_t index = 0;
  while (index < len) {
    uint64_t session_id = voyager::DecodeFixed64(s);
    uint64_t instance_id = voyager::DecodeFixed64(s + 8);
    sessions_[group_id].insert(std::make_pair(session_id, instance_id));
    s += 16;
    index += 16;
  }
  assert(index == len);
  assert(size == sessions_[group_id].size());
}

void SaberDB::SerializeToString(
    const std::unordered_map<uint64_t, uint64_t>& sessions,
    std::string* s) const {
  voyager::PutFixed64(s, sessions.size());
  for (auto& it : sessions) {
    voyager::PutFixed64(s, it.first);
    voyager::PutFixed64(s, it.second);
  }
}

}  // namespace saber
