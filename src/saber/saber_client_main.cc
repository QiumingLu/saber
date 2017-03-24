// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <saber/client/saber_client.h>

int main() {
  voyager::EventLoop loop;
  saber::SaberClient client(&loop, std::string("127.0.0.1:8888"));
  client.Start();
  loop.Loop();
  return 0;
}
