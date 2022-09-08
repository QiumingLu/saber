// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_DB_H_
#define SABER_SERVER_SABER_DB_H_

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <skywalker/node.h>

#include "saber/proto/server.pb.h"
#include "saber/server/data_tree.h"
#include "saber/server/server_options.h"
#include "saber/server/session_manager.h"
#include "saber/util/runloop.h"

namespace saber {

class SaberDB : public skywalker::StateMachine {
 public:
  SaberDB(RunLoop* loop, const ServerOptions& options);
  virtual ~SaberDB();

  void Exists(uint32_t group_id, const ExistsRequest& request, Watcher* watcher,
              ExistsResponse* response) const;

  void GetData(uint32_t group_id, const GetDataRequest& request,
               Watcher* watcher, GetDataResponse* response) const;

  void GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                   Watcher* watcher, GetChildrenResponse* response) const;

  void CheckCreate(uint32_t group_id, const CreateRequest& request,
                   CreateResponse* response) const;

  void CheckDelete(uint32_t group_id, const DeleteRequest& request,
                   DeleteResponse* response) const;

  void CheckSetData(uint32_t group_id, const SetDataRequest& request,
                    SetDataResponse* response) const;

  void RemoveWatcher(uint32_t group_id, Watcher* watcher) const;

  bool FindSession(uint32_t group_id, uint64_t session_id,
                   uint64_t* version) const;

  bool FindSession(uint32_t group_id, uint64_t session_id,
                   uint64_t version) const;

  std::unordered_map<uint64_t, uint64_t> CopySessions(uint32_t group_id) const;

  virtual bool Recover(uint32_t group_id, uint64_t instance_id,
                       const std::string& dir);

  virtual bool Execute(uint32_t group_id, uint64_t instance_id,
                       const std::string& value, void* context = nullptr);

  virtual bool MakeCheckpoint(uint32_t group_id, uint64_t instance_id,
                              const std::string& dir,
                              const FinishCheckpointCallback& cb);

  virtual bool GetCheckpoint(uint32_t group_id, uint64_t instance_id,
                             const std::string& dir,
                             std::vector<std::string>* files);
 private:
  static constexpr const char* kDataCheckpoint = "SABERDATA.db";
  static constexpr const char* kSessionCheckpoint = "SABERSESSION.db";

  void Create(uint32_t group_id, const CreateRequest& request,
              const Transaction* txn, CreateResponse* response) const;

  void Delete(uint32_t group_id, const DeleteRequest& request,
              const Transaction* txn, DeleteResponse* response) const;

  void SetData(uint32_t group_id, const SetDataRequest& request,
               const Transaction* txn, SetDataResponse* response) const;

  bool CreateSession(uint32_t group_id, uint64_t session_id,
                     uint64_t new_version, uint64_t old_version) const;
  bool CloseSession(uint32_t group_id, uint64_t session_id,
                    uint64_t version) const;
  void KillSession(uint32_t group_id, uint64_t session_id,
                   const Transaction* txn) const;

  std::vector<std::unique_ptr<DataTree>> trees_;
  std::vector<std::unique_ptr<SessionManager>> sessions_;

  RunLoop* loop_;

  // No copying allowed
  SaberDB(const SaberDB&);
  void operator=(const SaberDB&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_DB_H_
