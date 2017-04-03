// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

namespace saber {

SaberDB::SaberDB(uint32_t group_size) {
  for (uint32_t i = 0; i < group_size; ++i) {
    trees_.push_back(std::unique_ptr<DataTree>(new DataTree()));
  }
}

SaberDB::~SaberDB() {
}

void SaberDB::Create(uint32_t group_id, const CreateRequest& request,
                    CreateResponse* response) {
  trees_[group_id]->Create(request, response);
}
  
void SaberDB::Delete(uint32_t group_id, const DeleteRequest& request,
                     DeleteResponse* response) {
  trees_[group_id]->Delete(request, response);
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
                      SetDataResponse* response) {
  trees_[group_id]->SetData(request, response);
}
 
void SaberDB::GetACL(uint32_t group_id, const GetACLRequest& request,
                     GetACLResponse* response) {
  trees_[group_id]->GetACL(request, response);
}
 
void SaberDB::SetACL(uint32_t group_id, const SetACLRequest& request, 
                     SetACLResponse* response) {
  trees_[group_id]->SetACL(request, response);
}
  
void SaberDB::GetChildren(uint32_t group_id, const GetChildrenRequest& request,
                          Watcher* watcher, GetChildrenResponse* response) {
  trees_[group_id]->GetChildren(request, watcher, response);
}
 
}  // namespace saber
