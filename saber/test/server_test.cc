// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <saber/util/logging.h>
#include <skywalker/node.h>
#include <voyager/core/eventloop.h>
#include <voyager/util/string_util.h>
#include "saber/server/saber_server.h"

int main(int argc, char** argv) {
  char path[1024];
  if (getcwd(path, sizeof(path)) == nullptr) {
    LOG_ERROR("getcwd failed.");
    return -1;
  }

  saber::ServerOptions server_options;
  server_options.paxos_group_size = 1;
  server_options.log_storage_path = std::string(path) + "/log";
  server_options.checkpoint_storage_path = std::string(path) + "/checkpoint";

  std::vector<std::string> server;

  std::string server_str = "1:127.0.0.1:6666:5666";
  voyager::SplitStringUsing(server_str, ":", &server);

  saber::ServerMessage server_message;
  server_message.id = atoi(server[0].data());
  server_message.host = server[1];
  server_message.client_port = atoi(server[2].data());
  server_message.paxos_port = atoi(server[3].data());

  server_options.all_server_messages.push_back(server_message);

  voyager::EventLoop loop;
  saber::SaberServer saber_server(&loop, server_options);

  bool res = saber_server.Start();
  if (res) {
    LOG_INFO("SaberServer start successful!");
    loop.Loop();
  } else {
    LOG_ERROR("SaberServer start failed!");
  }
  return 0;
}
