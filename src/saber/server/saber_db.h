// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_DB_H_
#define SABER_SERVER_SABER_DB_H_

#include "saber/server/data_tree.h"
#include "saber/proto/server.pb.h"

namespace saber {

class SaberDB {
 public:
  SaberDB(uint32_t group_size);
  ~SaberDB();

  void Create(uint32_t group_id, const CreateRequest& request, 
              const Transaction& txn, CreateResponse* response);

  void Delete(uint32_t group_id, const DeleteRequest& request,
              const Transaction& txn, DeleteResponse* response);

  void Exists(uint32_t group_id, const ExistsRequest& request,
                Watcher* watcher, ExistsResponse* response);
 
  void GetData(uint32_t group_id, const GetDataRequest& request, 
               Watcher* watcher, GetDataResponse* response);

  void SetData(uint32_t group_id, const SetDataRequest& request,
               const Transaction& txn, SetDataResponse* response);

  void GetACL(uint32_t group_id, const GetACLRequest& request,
              GetACLResponse* response);

  void SetACL(uint32_t group_id, const SetACLRequest& request, 
              const Transaction& txn, SetACLResponse* response); 

  void GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                   Watcher* watcher, GetChildrenResponse* response);
  
 private:
  std::vector<std::unique_ptr<DataTree> > trees_;

  // No copying allowed
  SaberDB(const SaberDB&);
  void operator=(const SaberDB&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_DB_H_
