// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_db.h"

namespace saber {

SaberDB::SaberDB() {
}

SaberDB::~SaberDB() {
}

void SaberDB::GetData(const std::string& path, Watcher* watcher,
                       GetDataResponse* response) {
  tree_.GetData(path, watcher, response);
}

}  // namespace saber
