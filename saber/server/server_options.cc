// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_options.h"

namespace saber {

ServerOptions::ServerOptions()
    : server_id(0),
      server_ip("127.0.0.1"),
      server_port(8888),
      server_thread_size(1) {
}

}  // namespace saber
