// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SERVER_OPTIONS_H_
#define SABER_SERVER_SERVER_OPTIONS_H_

#include <stdint.h>
#include <string>

namespace saber {

struct ServerOptions {
  uint16_t server_id;
  std::string server_ip;
  uint16_t server_port;
  int server_thread_size;

  ServerOptions();
};

}  // namespace saber

#endif  // SABER_SERVER_SERVER_OPTIONS_H_
