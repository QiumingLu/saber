// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVICE_ACL_H_
#define SABER_SERVICE_ACL_H_

namespace saber {

enum Permissions {
  kRead = 1 << 0,
  kWrite = 1 << 1,
  kCreate = 1 << 2,
  kDelete = 1 << 3,
  kAdmin = 1 << 4,
  kAll = kRead | kWrite | kCreate | kDelete | kAdmin,
};

}  // namespace saber

#endif  // SABER_SERVICE_ACL_H_
