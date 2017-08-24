// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

#include <stdlib.h>

#include <algorithm>

#include <skywalker/file.h>
#include <voyager/util/string_util.h>

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
      instances_(options.paxos_group_size),
      file_map_(options.paxos_group_size) {
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
  std::vector<std::string> result;
  std::vector<uint64_t> temp;
  for (uint32_t i = 0; i < trees_.size(); ++i) {
    s.clear();
    files.clear();
    result.clear();
    temp.clear();

    std::string dir = checkpoint_storage_path_ + "g" + std::to_string(i);
    skywalker::FileManager::Instance()->CreateDir(dir);
    skywalker::FileManager::Instance()->GetChildren(dir, &files, true);

    for (auto& file : files) {
      if (file == "g") {
        skywalker::ReadFileToString(skywalker::FileManager::Instance(),
                                    dir + "/" + file, &s);
        voyager::SplitStringUsing(s, "\r\n", &result);
        for (auto& one : result) {
          size_t found = one.find_first_of(":");
          uint64_t f = strtoull(one.substr(0, found).c_str(), &end, 10);
          uint64_t id = strtoull(one.substr(found + 1).c_str(), &end, 10);
          file_map_[i].push_back(std::make_pair(f, id));
        }
      } else {
        temp.push_back(strtoull(file.c_str(), &end, 10));
      }
    }

    std::sort(file_map_[i].begin(), file_map_[i].end());
    std::sort(temp.begin(), temp.end());
    assert(file_map_[i].size() == temp.size() ||
           file_map_[i].size() + 1 == temp.size());
    for (size_t j = 0; j < file_map_[i].size(); ++j) {
      if (file_map_[i][j].first != temp[j]) {
        LOG_ERROR("Checkpoint file error! the file is %llu",
                  (unsigned long long)temp[j]);
        return false;
      }
    }

    if (!(file_map_[i].empty())) {
      if (file_map_[i].back().first != temp.back()) {
        temp.pop_back();
        skywalker::FileManager::Instance()->DeleteFile(
            dir + "/" + std::to_string(temp.back()));
      }
      ReadFileToString(skywalker::FileManager::Instance(),
                       dir + "/" + std::to_string(file_map_[i].back().first),
                       &s);
      trees_[i]->Recover(s);
    }
  }
  loop_ = thread_.Loop();
  return true;
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

void SaberDB::RemoveWatcher(Watcher* watcher) {
  for (auto& tree : trees_) {
    tree->RemoveWatcher(watcher);
  }
}

bool SaberDB::Execute(uint32_t group_id, uint64_t instance_id,
                      const std::string& value, void* context) {
  SaberMessage message;
  message.ParseFromString(value);
  char* end;
  uint64_t timestamp = strtoull(message.extra_data().c_str(), &end, 10);
  Transaction txn;
  txn.set_time(timestamp);
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

  bool expected = false;
  if (doing_.compare_exchange_strong(expected, true)) {
    uint64_t i = file_map_[group_id].empty()
                     ? UINTMAX_MAX
                     : file_map_[group_id].back().second;
    if ((i == UINTMAX_MAX && instance_id > kMakeCheckpointInterval) ||
        (instance_id - i > kMakeCheckpointInterval)) {
      loop_->QueueInLoop([this, group_id]() { MakeCheckpoint(group_id); });
    } else {
      doing_ = false;
    }
  }
  instances_[group_id] = instance_id;
  return true;
}

void SaberDB::MakeCheckpoint(uint32_t group_id) {
  if (LockCheckpoint(group_id)) {
    std::string s;
    trees_[group_id]->SerializeToString(&s);
    uint64_t id = instances_[group_id];
    uint64_t f = NowMillis();
    std::string fname = checkpoint_storage_path_ + "g" +
                        std::to_string(group_id) + "/" + std::to_string(f);
    skywalker::Status status = skywalker::WriteStringToFileSync(
        skywalker::FileManager::Instance(), s, fname);
    if (status.ok()) {
      file_map_[group_id].push_back(std::make_pair(f, id));
      LOG_INFO(
          "make checkpoint successful! the file is %s, instance_id is %llu",
          fname.c_str(), (unsigned long long)id);
      CleanCheckpoint(group_id);
    } else {
      skywalker::FileManager::Instance()->DeleteFile(fname);
    }
    UnLockCheckpoint(group_id);
  }
  doing_ = false;
}

void SaberDB::CleanCheckpoint(uint32_t group_id) {
  while (file_map_[group_id].size() > kKeepCheckpointCount) {
    std::string del = checkpoint_storage_path_ + "g" +
                      std::to_string(group_id) + "/" +
                      std::to_string(file_map_[group_id].front().first);
    skywalker::FileManager::Instance()->DeleteFile(del);
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
    std::string fname =
        checkpoint_storage_path_ + "g" + std::to_string(group_id) + "/g";
    skywalker::FileManager::Instance()->DeleteFile(fname);
    skywalker::WriteStringToFileSync(skywalker::FileManager::Instance(), s,
                                     fname);
  }
}

uint64_t SaberDB::GetCheckpointInstanceId(uint32_t group_id) {
  return file_map_[group_id].empty() ? UINTMAX_MAX
                                     : file_map_[group_id].back().second;
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
    files->push_back(std::to_string(file_map_[group_id].back().first));
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

  file_map_[group_id].push_back(std::make_pair(f, instance_id));

  LOG_INFO("load checkpoint successful! the file is %s, instance_id is %llu",
           fname.c_str(), (unsigned long long)instance_id);
  CleanCheckpoint(group_id);

  return true;
}

}  // namespace saber
