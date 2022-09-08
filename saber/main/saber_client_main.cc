// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <saber/util/logging.h>
#include <saber/util/runloop.h>

#include "saber/main/default_watcher.h"
#include "saber/main/mysaber.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s server_ip:server_port,...\n", argv[0]);
    return -1;
  }

  // saber::SetLogHandler(nullptr);
  saber::DefaultWatcher watcher;
  saber::ClientOptions options;
  options.root = "/ls";
  options.servers = argv[1];
  options.watcher = &watcher;
  saber::RunLoop loop;
  saber::MySaber mysaber(&loop, options);
  mysaber.Start();
  loop.Loop();
  return 0;
}
