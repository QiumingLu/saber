// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"

#include <voyager/core/tcp_connection.h>

#include "saber/server/saber_cell.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

std::atomic<uint8_t> SaberServer::seq_num_;

SaberServer::SaberServer(voyager::EventLoop* loop,
                         const ServerOptions& options)
    : options_(options),
      server_id_(options_.my_server_message.server_id),
      addr_(options.my_server_message.server_ip,
            options.my_server_message.client_port),
      server_(loop, addr_, "SaberServer", options.server_thread_size) {
}

SaberServer::~SaberServer() {
}

bool SaberServer::Start() {
  skywalker::GroupOptions group_options;
  group_options.use_master = true;
  group_options.log_sync = true;
  group_options.sync_interval = 3;
  group_options.keep_log_count = 10000;
  group_options.log_storage_path = options_.log_storage_path;

  for (auto& server_message : options_.all_server_messages) {
    SaberCell::Instance()->AddServer(server_message);
    group_options.membership.push_back(
        skywalker::IpPort(server_message.server_ip,
                          server_message.paxos_port));
  }

  db_.reset(new SaberDB(3));
  db_->set_machine_id(6);

  group_options.checkpoint = db_.get();
  group_options.machines.push_back(db_.get());

  skywalker::Options skywalker_options;
  skywalker_options.ipport.ip = options_.my_server_message.server_ip;
  skywalker_options.ipport.port = options_.my_server_message.paxos_port;

  for (int i = 0; i < 3; ++i) {
    group_options.group_id = i;
    skywalker_options.groups.push_back(group_options);
  }

  skywalker::Node* node;
  bool res = skywalker::Node::Start(skywalker_options, &node);
  if (res) {
    LOG_INFO("Skywalker start successfully!");
    node_.reset(node);

    server_.SetConnectionCallback([this](const voyager::TcpConnectionPtr& p) {
      OnConnection(p);
    });
    server_.SetCloseCallback([this](const voyager::TcpConnectionPtr& p) {
      OnClose(p);
    });
    server_.Start();
  } else {
    LOG_ERROR("Skywalker start failed!");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn = new ServerConnection(
      GetNextSessionId(), p, db_.get(), node_.get());
  bool result = conns_.insert(conn->session_id(),
                              std::unique_ptr<ServerConnection>(conn));
  if (result) {
    p->SetContext(conn);
    p->SetMessageCallback(
        [conn](const voyager::TcpConnectionPtr& ptr, voyager::Buffer* buf) {
      conn->OnMessage(ptr, buf);
    });
  } else {
    p->ShutDown();
  }
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  if (conn != nullptr) {
    db_->RemoveWatcher(conn);
    conns_.erase(conn->session_id());
  }
}

uint64_t SaberServer::GetNextSessionId() const {
  uint64_t session_id = (NowMillis() << 22) | (server_id_ << 8) | seq_num_++;
  return session_id;
}

}  // namespace saber
