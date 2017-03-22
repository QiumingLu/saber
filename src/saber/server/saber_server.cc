// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"

#include <voyager/core/tcp_connection.h>
#include "saber/util/logging.h"

namespace saber {

SaberServer::SaberServer(voyager::EventLoop* loop,
                         const voyager::SockAddr& addr, int thread_size)
    : server_(loop, addr, "SaberServer", thread_size) {
}

SaberServer::~SaberServer() {
}

bool SaberServer::Start(const skywalker::Options& options) {
  server_.SetConnectionCallback([this](const voyager::TcpConnectionPtr& p) {
    OnConnection(p);
  });
  server_.SetCloseCallback([this](const voyager::TcpConnectionPtr& p) {
    OnClose(p);
  });
  server_.SetMessageCallback(
      [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
    OnMessage(p, buf);
  });

  bool res = skywalker::Node::Start(options, &node_);
  if (res) {
    server_.Start();
    LOG_INFO("SaberServer start successfully!\n");
  } else {
    LOG_INFO("Skywalker start failed!\n");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn = new ServerConnection(p, &db_);
  conn->SetSessionId(seq_.GetNext());
  p->SetContext(conn);
  conns_.insert(conn->SessionId(), std::unique_ptr<ServerConnection>(conn));
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  conns_.erase(conn->SessionId());
}

void SaberServer::OnMessage(
    const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  conn->OnMessage(buf);
}

}  // namespace saber
