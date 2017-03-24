// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_DATA_BASE_H_
#define SABER_SERVER_DATA_BASE_H_

#include "saber/server/data_tree.h"

namespace saber {

class DataBase {
 public:
  DataBase();
  ~DataBase();

 private:
  DataTree tree_;

  // No copying allowed
  DataBase(const DataBase&);
  void operator=(const DataBase&);
};

}  // namespace saber

#endif  // SABER_SERVER_DATA_BASE_H_
