// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"
#include "saber/server/saber_cell.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/server/connection_monitor.h"
#include "saber/util/logging.h"

namespace saber {

SaberServer::SaberServer(voyager::EventLoop* loop,
                         const ServerOptions& options)
    : options_(options),
      server_id_(options_.my_server_message.server_id),
      addr_(options.my_server_message.server_ip,
            options.my_server_message.client_port),
      server_(loop, addr_, "SaberServer", options.paxos_group_size) {
}

SaberServer::~SaberServer() {
}

bool SaberServer::Start() {
  skywalker::GroupOptions group_options;
  group_options.use_master = true;
  group_options.log_sync = true;
  group_options.sync_interval = options_.log_sync_interval;
  group_options.keep_log_count = options_.keep_log_count;
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

  for (int i = 0; i < options_.paxos_group_size; ++i) {
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

    const std::vector<voyager::EventLoop*>* loops = server_.AllLoops();
    for (size_t i = 0; i < loops->size(); ++i) {
      ConnectionMonitor* monitor = new ConnectionMonitor(
          server_id_,
          options_.max_ip_connections, options_.max_all_connections,
          db_.get(), node_.get());
      monitors_[loops->at(i)] = std::unique_ptr<ConnectionMonitor>(monitor);
    }
  } else {
    LOG_ERROR("Skywalker start failed!");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  monitors_[p->OwnerEventLoop()]->OnConnection(p);
  /*
  ServerConnection* conn = new ServerConnection(
      GetNextSessionId(), p, db_.get(), node_.get());
  p->SetContext(conn);
  p->SetMessageCallback(
      [conn](const voyager::TcpConnectionPtr& ptr, voyager::Buffer* buf) {
    conn->OnMessage(ptr, buf);
  });
  */
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  monitors_[p->OwnerEventLoop()]->OnClose(p);
  /*
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  assert(conn != nullptr);
  db_->RemoveWatcher(conn);
  delete conn;
  */
}

/*
uint64_t SaberServer::GetNextSessionId() const {
  static SequencenNumber<int> seq_num_(1 << 10);
  return (NowMillis() << 22) | (server_id_ << 10) | seq_num_.GetNext();
}
*/

}  // namespace saber
