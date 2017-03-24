// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include <voyager/core/eventloop.h>
#include <voyager/core/sockaddr.h>
#include <voyager/core/tcp_client.h>

#include "saber/client/callbacks.h"
#include "saber/client/server_manager.h"

namespace saber {

class SaberClient {
 public:
  SaberClient(
      voyager::EventLoop* loop, 
      const std::string& server, 
      std::unique_ptr<ServerManager> p = std::unique_ptr<ServerManager>());

  ~SaberClient();

  void Start();

 private:
  void Connect();
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  void OnMessage(const voyager::TcpConnectionPtr& p, voyager::Buffer* buf);

  voyager::EventLoop* loop_;
  std::unique_ptr<voyager::TcpClient> client_;

  std::unique_ptr<ServerManager> server_manager_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
