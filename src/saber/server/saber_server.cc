// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"

#include <voyager/core/tcp_connection.h>

#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

SaberServer::SaberServer(voyager::EventLoop* loop,
                         const ServerOptions& options)
    : server_id_(options.server_id),
      addr_(options.server_ip, options.server_port),
      server_(loop, addr_, "SaberServer", options.server_thread_size) {
}

SaberServer::~SaberServer() {
}

bool SaberServer::Start(const skywalker::Options& options) {
  skywalker::Node* node;
  bool res = skywalker::Node::Start(options, &node);
  if (res) {
    LOG_INFO("Skywalker start successfully!\n");
    db_.reset(new SaberDB(options.group_size));
    db_->set_machine_id(6);
    node_.reset(node);
    node_->AddMachine(db_.get());

    server_.SetConnectionCallback([this](const voyager::TcpConnectionPtr& p) {
      OnConnection(p);
    });
    server_.SetCloseCallback([this](const voyager::TcpConnectionPtr& p) {
      OnClose(p);
    });
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
      p->OwnerEventLoop(), std::unique_ptr<Messager>(messager),
      db_.get(), node_.get());
  p->SetContext(conn);
  conn->set_session_id(GetNextSessionId());
  conns_.insert(conn->session_id(), std::unique_ptr<ServerConnection>(conn));
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  db_->RemoveWatch(conn);
  conns_.erase(conn->session_id());
}

uint64_t SaberServer::GetNextSessionId() const {
  return ((NowMillis() << 16) | server_id_);
}

}  // namespace saber
