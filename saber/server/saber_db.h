// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_DB_H_
#define SABER_SERVER_SABER_DB_H_

#include <vector>
#include <string>
#include <memory>

#include <skywalker/node.h>

#include "saber/server/data_tree.h"
#include "saber/proto/server.pb.h"

namespace saber {

class SaberDB : public skywalker::StateMachine, public skywalker::Checkpoint {
 public:
  explicit SaberDB(uint32_t group_size);
  virtual ~SaberDB();

  void Exists(uint32_t group_id, const ExistsRequest& request,
              Watcher* watcher, ExistsResponse* response);

  void GetData(uint32_t group_id, const GetDataRequest& request,
               Watcher* watcher, GetDataResponse* response);

  void GetACL(uint32_t group_id, const GetACLRequest& request,
              GetACLResponse* response);

  void GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                   Watcher* watcher, GetChildrenResponse* response);

  void RemoveWatcher(Watcher* watcher);

  virtual bool Execute(uint32_t group_id,
                       uint64_t instance_id,
                       const std::string& value,
                       skywalker::MachineContext* context);

  virtual uint64_t GetCheckpointInstanceId(uint32_t group_id);

  virtual bool LockCheckpoint(uint32_t group_id);

  virtual bool UnLockCheckpoint(uint32_t group_id);

  virtual bool GetCheckpoint(uint32_t group_id, int machine_id,
                             std::string* dir,
                             std::vector<std::string>* files);

  virtual bool LoadCheckpoint(uint32_t group_id, int machine_id,
                              const std::string& dir,
                              const std::vector<std::string>& files);

 private:
  void Create(uint32_t group_id, const CreateRequest& request,
              const Transaction& txn, CreateResponse* response);

  void Delete(uint32_t group_id, const DeleteRequest& request,
              const Transaction& txn, DeleteResponse* response);

  void SetData(uint32_t group_id, const SetDataRequest& request,
               const Transaction& txn, SetDataResponse* response);

  void SetACL(uint32_t group_id, const SetACLRequest& request,
              const Transaction& txn, SetACLResponse* response);

  std::vector<std::unique_ptr<DataTree> > trees_;

  // No copying allowed
  SaberDB(const SaberDB&);
  void operator=(const SaberDB&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_DB_H_
