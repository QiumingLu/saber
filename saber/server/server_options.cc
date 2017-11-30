// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_options.h"

namespace saber {

ServerMessage::ServerMessage()
    : id(1), host("127.0.0.1"), client_port(6666), paxos_port(5666) {}

ServerOptions::ServerOptions()
    : server_thread_size(2),
      paxos_io_thread_size(2),
      paxos_callback_thread_size(2),
      paxos_group_size(10),
      tick_time(3000000),
      session_timeout(4 * tick_time),
      max_all_connections(60000),
      max_ip_connections(60),
      max_data_size(1024 * 1024),
      keep_log_count(1000000),
      log_sync_interval(10),
      keep_checkpoint_count(3),
      make_checkpoint_interval(200000),
      async_serialize_checkpoint_data(true) {}

}  // namespace saber
