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

void SaberDB::CreateNode(uint32_t group_id, const std::string& path, 
                         const std::string& data, CreateResponse* response) {
  trees_[group_id]->CreateNode(path, data, response);
}
  
void SaberDB::DeleteNode(uint32_t group_id, const std::string& path,
                         DeleteResponse* response) {
  trees_[group_id]->DeleteNode(path, response);
}
  
void SaberDB::SetData(uint32_t group_id, const std::string& path, 
                      const std::string& data, SetDataResponse* response) {
  trees_[group_id]->SetData(path, data, response);
}
 
void SaberDB::GetData(uint32_t group_id, const std::string& path, 
                      Watcher* watcher, GetDataResponse* response) {
  trees_[group_id]->GetData(path, watcher, response);
}

}  // namespace saber
