// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_OPTIONS_H_
#define SABER_SERVER_SERVER_OPTIONS_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace saber {

struct ServerMessage {
  // Default: 1 between [0, 4096)
  uint64_t id;

  // Default: "127.0.0.1"
  std::string host;

  // Default: 6666
  uint16_t client_port;

  // Default: 5666
  uint16_t paxos_port;

  ServerMessage();
};

struct ServerOptions {
  // Default: 2
  uint32_t server_thread_size;

  // Default: 2
  uint32_t paxos_io_thread_size;

  // Default: 10
  uint32_t paxos_group_size;

  // Default: 3000
  uint32_t tick_time;

  // Default: 2 * tick_time
  uint32_t min_session_timeout;

  // Default: 20 * tick_time
  uint32_t max_session_timeout;

  // Default: 60000
  uint32_t max_all_connections;

  // Default: 60
  uint32_t max_ip_connections;

  // Default: 1024 * 1024
  uint32_t max_data_size;

  // Default: 1000000
  uint32_t keep_log_count;

  // Default: 10
  uint32_t log_sync_interval;

  // Default: 3
  uint32_t keep_checkpoint_count;

  // Default: 200000
  uint32_t make_checkpoint_interval;

  // Default: ""
  std::string log_storage_path;

  // Default: ""
  std::string checkpoint_storage_path;

  ServerMessage my_server_message;
  std::vector<ServerMessage> all_server_messages;

  ServerOptions();
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_OPTIONS_H_
