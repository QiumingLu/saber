// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <unistd.h>

#include <saber/server/saber_server.h>
#include <saber/util/logging.h>
#include <skywalker/logging.h>
#include <voyager/core/eventloop.h>
#include <voyager/util/logging.h>
#include <voyager/util/string_util.h>

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Usage: %s id:ip:port:port id:ip:port:port,...\n", argv[0]);
    return -1;
  }
  char path[1024];
  if (getcwd(path, sizeof(path)) == nullptr) {
    printf("getcwd failed.\n");
    return -1;
  }

  saber::ServerOptions server_options;
  server_options.log_storage_path = std::string(path) + "/log";
  server_options.checkpoint_storage_path = std::string(path) + "/checkpoint";

  std::vector<std::string> server;
  std::vector<std::string> servers;

  voyager::SplitStringUsing(std::string(argv[1]), ":", &server);
  if (server.size() != 4) {
    printf("Usage: %s id:ip:port:port id:ip:port:port,...\n", argv[0]);
    return -1;
  }

  server_options.my_server_message.id = atoi(server[0].data());
  server_options.my_server_message.host = server[1];
  server_options.my_server_message.client_port = atoi(server[2].data());
  server_options.my_server_message.paxos_port = atoi(server[3].data());

  // Just for test
  // server_options.keep_log_count = 20;
  // server_options.keep_checkpoint_count = 3;
  // server_options.make_checkpoint_interval = 5;

  voyager::SplitStringUsing(std::string(argv[2]), ",", &servers);
  for (auto& s : servers) {
    server.clear();
    voyager::SplitStringUsing(s, ":", &server);
    if (server.size() != 4) {
      printf("Usage: %s id:ip:port:port id:ip:port:port,...\n", argv[0]);
      return -1;
    }

    saber::ServerMessage server_message;
    server_message.id = atoi(server[0].data());
    server_message.host = server[1];
    server_message.client_port = atoi(server[2].data());
    server_message.paxos_port = atoi(server[3].data());

    server_options.all_server_messages.push_back(server_message);
  }

  voyager::EventLoop loop;
  saber::SaberServer saber_server(&loop, server_options);

  voyager::SetLogLevel(voyager::LOGLEVEL_ERROR);
  skywalker::SetLogLevel(skywalker::LOGLEVEL_WARN);
  saber::SetLogLevel(saber::LOGLEVEL_INFO);

  bool res = saber_server.Start();
  if (res) {
    printf("--------------------------------------------------------------\n");
    printf("SaberServer start successful!\n");
    printf("--------------------------------------------------------------\n");
    loop.Loop();
  } else {
    printf("--------------------------------------------------------------\n");
    printf("SaberServer start failed!\n");
    printf("--------------------------------------------------------------\n");
  }
  return 0;
}
