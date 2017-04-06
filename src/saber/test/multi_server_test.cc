// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <voyager/core/eventloop.h>
#include <voyager/util/string_util.h>
#include <skywalker/node.h>
#include <skywalker/logging.h>
#include <saber/util/logging.h>
#include <saber/server/saber_server.h>
#include <saber/server/saber_cell.h>
#include <iostream>
int main(int argc, char** argv) {
  if (argc != 3) {
    LOG_ERROR("Usage: %s id:ip:port:port id:ip:port:port,...\n", argv[0]);
    return -1;
  }
  char path[1024];
  if (getcwd(path, sizeof(path)) == nullptr) {
    LOG_ERROR("getcwd failed.\n");
    return -1;
  }

  skywalker::Options skywalker_options;
  saber::ServerOptions server_options;

  skywalker_options.log_storage_path = std::string(path);
  skywalker_options.log_sync = true;
  skywalker_options.sync_interval = 3;
  skywalker_options.group_size = 1;

  std::vector<std::string> my;
  voyager::SplitStringUsing(std::string(argv[1]), ":", &my);
  assert(my.size() == 4);
  skywalker_options.ipport.ip = my[1];
  skywalker_options.ipport.port = atoi(my[3].data());

  server_options.server_id = atoi(my[0].data());
  server_options.server_ip = my[1];
  server_options.server_port = atoi(my[2].data());
  server_options.server_thread_size = 4;

  std::vector<std::string> servers;
  voyager::SplitStringUsing(std::string(argv[2]), ",", &servers);
  for (auto& s : servers) {
    std::vector<std::string> server;
    voyager::SplitStringUsing(s, ":", &server);
    std::cout <<s  << servers.size()<< std::endl;
    assert(servers.size() == 4);
    skywalker_options.membership.push_back(
        skywalker::IpPort(server[1], atoi(server[3].data())));
    saber::ServerMessage m;
    m.server_id = atoi(server[0].data());
    m.client_port = atoi(server[2].data());
    m.paxos_port = atoi(server[3].data());
    m.ip = server[1];
    saber::SaberCell::Instance()->AddServer(m);
  }
  skywalker::SetLogHandler(nullptr);

  voyager::EventLoop loop;
  saber::SaberServer server(&loop, server_options);

  bool res = server.Start(skywalker_options);
  if (res) {
    LOG_INFO("SaberServer start successfully!\n");
    loop.Loop();
  } else {
    LOG_ERROR("SaberServer start failed!\n");
  }
  return 0;
}
