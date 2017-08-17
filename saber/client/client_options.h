// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_OPTIONS_H_
#define SABER_CLIENT_SABER_CLIENT_OPTIONS_H_

#include <string>
#include "saber/client/server_manager.h"

namespace saber {

struct ClientOptions {
  bool auto_watch_reset;
  std::string root;
  std::string servers;
  ServerManager* server_manager;

  ClientOptions();
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_OPTIONS_H_
