// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_options.h"

namespace saber {

ServerMessage::ServerMessage()
    : server_id(1),
      server_ip("127.0.0.1"),
      client_port(6666),
      paxos_port(5666) {
}

ServerOptions::ServerOptions()
    : server_thread_size(1) {
}

}  // namespace saber
