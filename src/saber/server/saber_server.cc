// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"

#include <voyager/core/tcp_connection.h>

#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

SaberServer::SaberServer(uint16_t server_id, voyager::EventLoop* loop,
                         const voyager::SockAddr& addr, int thread_size)
    : server_id_(server_id),
      server_(loop, addr, "SaberServer", thread_size) {
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

  skywalker::Node* node;
  bool res = skywalker::Node::Start(options, &node);
  if (res) {
    LOG_INFO("Skywalker start successfully!\n");
    db_.reset(new SaberDB(options.group_size));
    node_.reset(node);
    server_.Start();
  } else {
    LOG_ERROR("Skywalker start failed!\n");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  Messager* messager = new Messager();
  messager->SetTcpConnection(p);
  p->SetMessageCallback(
      [messager](const voyager::TcpConnectionPtr& ptr, voyager::Buffer* buf) {
    messager->OnMessage(ptr, buf);
  });
  ServerConnection* conn = new ServerConnection(
      std::unique_ptr<Messager>(messager),
      db_.get(), p->OwnerEventLoop(), node_.get());
  p->SetContext(conn);
  conn->set_session_id(GetNextSessionId());
  conns_.insert(conn->session_id(), std::unique_ptr<ServerConnection>(conn));
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  conns_.erase(conn->session_id());
}

uint64_t SaberServer::GetNextSessionId() const {
  return ((NowMillis() << 16) | server_id_);
}

}  // namespace saber
