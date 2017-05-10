// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_STORAGE_DB_H_
#define SABER_STORAGE_DB_H_

namespace saber {

class DB {
 public:
  DB();
  ~DB();

 private:
  DB(const DB&);
  void operator=(const DB&);
};

}  // namespace saber

#endif  // SABER_STORAGE_DB_H_
