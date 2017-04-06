// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <voyager/core/eventloop.h>
#include <voyager/util/string_util.h>
#include <skywalker/node.h>
#include <saber/util/logging.h>
#include <saber/server/saber_server.h>

int main(int argc, char** argv) {
  if (argc != 5) {
    LOG_ERROR("Usage: %s server_id server_ip:server_port "
              "myip:myport node0_ip:node0_port,...\n", argv[0]);
    return -1;
  }
  char path[1024];
  if (getcwd(path, sizeof(path)) == nullptr) {
    LOG_ERROR("getcwd failed.\n");
    return -1;
  }
  skywalker::Options options;
  options.log_storage_path = std::string(path);
  options.log_sync = true;
  options.sync_interval = 3;
  options.group_size = 3;
  std::vector<std::string> my;
  voyager::SplitStringUsing(std::string(argv[3]), ":", &my);
  options.ipport.ip = my[0];
  options.ipport.port = atoi(my[1].data());

  std::vector<std::string> nodes;
  voyager::SplitStringUsing(std::string(argv[4]), ",", &nodes);
  for (auto& i : nodes) {
    std::vector<std::string> node;
    voyager::SplitStringUsing(i, ":", &node);
    options.membership.push_back(
        skywalker::IpPort(node[0], atoi(node[1].data())));
  }

  voyager::EventLoop loop;
  saber::ServerOptions server_options;
  server_options.server_id = atoi(argv[1]);
  std::vector<std::string> s;
  voyager::SplitStringUsing(std::string(argv[2]), ":", &s);
  server_options.server_ip = s[0];
  server_options.server_port = atoi(s[1].data());
  server_options.server_thread_size = 4;
  saber::SaberServer server(&loop, server_options);

  bool res = server.Start(options);
  if (res) {
    LOG_INFO("SaberServer start successfully!\n");
    loop.Loop();
  } else {
    LOG_ERROR("SaberServer start failed!\n");
  }
  return 0;
}
