// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_OPTIONS_H_
#define SABER_SERVER_SERVER_OPTIONS_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace saber {

class ServerMessage {
 public:
  int server_id;
  std::string server_ip;
  uint16_t client_port;
  uint16_t paxos_port;

  ServerMessage();
};

struct ServerOptions {
  int paxos_group_size;
  int tick_time;
  int min_session_timeout;
  int max_session_timeout;
  int max_all_connections;
  int max_ip_connections;
  int max_data_size;
  int keep_log_count;
  int log_sync_interval;
  int keep_checkpoint_count;
  int make_checkpoint_interval;
  std::string log_storage_path;
  std::string checkpoint_storage_path;

  ServerMessage my_server_message;
  std::vector<ServerMessage> all_server_messages;

  ServerOptions();
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_OPTIONS_H_
