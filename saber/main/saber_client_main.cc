// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include "saber/main/mysaber.h"
#include "saber/util/logging.h"
#include "saber/util/runloop.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s server_ip:server_port,...\n", argv[0]);
    return -1;
  }

  saber::SetLogHandler(nullptr);
  saber::ClientOptions options;
  options.group_size = 1;
  options.servers = argv[1];
  saber::RunLoop loop;
  saber::MySaber mysaber(&loop, options);
  mysaber.Start();
  loop.Loop();
  return 0;
}
