// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_DB_H_
#define SABER_SERVER_SABER_DB_H_

#include "saber/server/data_tree.h"

namespace saber {

class SaberDB {
 public:
  SaberDB();
  ~SaberDB();

  void GetData(const std::string& path, Watcher* watcher,
               GetDataResponse* response);

 private:
  DataTree tree_;

  // No copying allowed
  SaberDB(const SaberDB&);
  void operator=(const SaberDB&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_DB_H_
