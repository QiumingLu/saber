// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_OPTIONS_H_
#define SABER_SERVER_SERVER_OPTIONS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <skywalker/cluster.h>

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
  // The saber's thread model is:
  //  ______________________________________________
  // | name                   |        size         |
  // |————————————————————————————————————————————--|
  // |                                              |
  // | main thread            |          1          |
  // |                                              |
  // | runloop thread         |          1          |
  // |                                              |
  // | voyager thread model   |          N          |
  // |                                              |
  // | skywalker thread model |          N          |
  // |                                              |
  //  -----------------------------------------------
  // The saber' thread size is:
  // 7 + server_thread_size + paxos_io_thread_size + paxos_callback_thread_size

  // Default: 3
  uint32_t server_thread_size;

  // Default: 2
  uint32_t paxos_io_thread_size;

  // Default: 1
  uint32_t paxos_callback_thread_size;

  // Default: 10
  uint32_t paxos_group_size;

  // Default: 3000ms
  uint32_t tick_time;

  // Default: 4 * tick_time
  uint32_t session_timeout;

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

  // Default: ""
  std::string log_storage_path;

  // Default: 3
  uint32_t keep_checkpoint_count;

  ServerMessage my_server_message;
  std::vector<ServerMessage> all_server_messages;

  skywalker::Cluster* cluster;

  ServerOptions();
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_OPTIONS_H_
