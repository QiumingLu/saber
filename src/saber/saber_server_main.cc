// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <voyager/core/eventloop.h>
#include <voyager/core/sockaddr.h>
#include <skywalker/node.h>
#include <saber/util/logging.h>
#include <saber/server/saber_server.h>

int main() {
  char path[1024];
  if (getcwd(path, sizeof(path)) == nullptr) {
    LOG_ERROR("getcwd failed.\n");
    return -1;
  }
  skywalker::Options options;
  options.log_storage_path = std::string(path);
  options.log_sync = true;
  options.sync_interval = 3;
  options.group_size = 1;
  options.use_master = true;
  skywalker::IpPort i("127.0.0.1", 8887);
  options.ipport = i;
  options.membership.push_back(i);

  voyager::EventLoop loop;
  voyager::SockAddr addr("127.0.0.1", 8888);
  saber::SaberServer server(&loop, addr);

  bool res = server.Start(options);
  if (res) {
    loop.Loop();
  }
  return 0;
}
