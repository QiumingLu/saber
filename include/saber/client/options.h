// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_OPTIONS_H_
#define SABER_CLIENT_SABER_OPTIONS_H_

#include <string>
#include "saber/client/server_manager.h"

namespace saber {

struct Options {
  bool auto_watch_reset;
  uint32_t group_size;
  std::string servers;
  ServerManager* server_manager;

  Options();
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_OPTIONS_H_
