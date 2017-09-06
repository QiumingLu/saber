// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_DB_H_
#define SABER_SERVER_SABER_DB_H_

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <skywalker/node.h>

#include "saber/proto/server.pb.h"
#include "saber/server/data_tree.h"
#include "saber/server/server_options.h"
#include "saber/server/session_manager.h"
#include "saber/util/mutex.h"
#include "saber/util/runloop.h"
#include "saber/util/runloop_thread.h"

namespace saber {

class SaberDB : public skywalker::StateMachine, public skywalker::Checkpoint {
 public:
  explicit SaberDB(const ServerOptions& options);
  virtual ~SaberDB();

  bool Recover();

  void Exists(uint32_t group_id, const ExistsRequest& request, Watcher* watcher,
              ExistsResponse* response);

  void GetData(uint32_t group_id, const GetDataRequest& request,
               Watcher* watcher, GetDataResponse* response);

  void GetACL(uint32_t group_id, const GetACLRequest& request,
              GetACLResponse* response);

  void GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                   Watcher* watcher, GetChildrenResponse* response);

  void RemoveWatcher(uint32_t group_id, Watcher* watcher);

  bool FindSession(uint64_t group_id, uint64_t session_id,
                   uint64_t* version) const;

  bool FindSession(uint64_t group_id, uint64_t session_id,
                   uint64_t version) const;

  virtual bool Execute(uint32_t group_id, uint64_t instance_id,
                       const std::string& value, void* context);

  virtual uint64_t GetCheckpointInstanceId(uint32_t group_id);

  virtual bool LockCheckpoint(uint32_t group_id);

  virtual bool UnLockCheckpoint(uint32_t group_id);

  virtual bool GetCheckpoint(uint32_t group_id, uint32_t machine_id,
                             std::string* dir, std::vector<std::string>* files);

  virtual bool LoadCheckpoint(uint32_t group_id, uint64_t instance_id,
                              uint32_t machine_id, const std::string& dir,
                              const std::vector<std::string>& files);

 private:
  bool Checksum(std::string* s) const;
  std::string FileName(uint32_t group_id, uint64_t instance_id) const;
  void DeleteFile(const std::string& fname) const;

  void Create(uint32_t group_id, const CreateRequest& request,
              const Transaction& txn, CreateResponse* response);

  void Delete(uint32_t group_id, const DeleteRequest& request,
              const Transaction& txn, DeleteResponse* response);

  void SetData(uint32_t group_id, const SetDataRequest& request,
               const Transaction& txn, SetDataResponse* response);

  void SetACL(uint32_t group_id, const SetACLRequest& request,
              const Transaction& txn, SetACLResponse* response);
  bool CreateSession(uint32_t group_id, uint64_t session_id,
                     uint64_t new_version, uint64_t old_version);
  bool CloseSession(uint32_t group_id, uint64_t session_id, uint64_t version);
  void KillSession(uint32_t group_id, uint64_t session_id,
                   const Transaction& txn);

  void MaybeMakeCheckpoint(uint32_t group_id, uint64_t instance_id);
  void MakeCheckpoint(
      uint32_t group_id, uint64_t instance_id,
      std::unordered_map<std::string, DataNode>* nodes,
      std::unordered_map<std::string, std::set<std::string>>* childrens,
      std::unordered_map<uint64_t, uint64_t>* sessions);
  void CleanCheckpoint(uint32_t group_id);

  const uint32_t kKeepCheckpointCount;
  const uint32_t kMakeCheckpointInterval;

  std::atomic<bool> lock_;
  std::atomic<bool> doing_;

  std::string checkpoint_storage_path_;
  std::vector<uint64_t> checkpoint_id_;
  std::vector<std::vector<uint64_t>> files_;
  std::vector<std::unique_ptr<DataTree>> trees_;
  std::vector<std::unique_ptr<SessionManager>> sessions_;

  std::default_random_engine generator_;
  std::uniform_int_distribution<uint32_t> distribution_;

  RunLoop* loop_;
  RunLoopThread thread_;

  // No copying allowed
  SaberDB(const SaberDB&);
  void operator=(const SaberDB&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_DB_H_
