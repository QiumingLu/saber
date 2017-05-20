// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/server/connection_monitor.h"
#include "saber/util/logging.h"
#include "saber/util/timeops.h"
#include "saber/util/sequence_number.h"

namespace saber {

SaberServer::SaberServer(voyager::EventLoop* loop,
                         const ServerOptions& options)
    : options_(options),
      server_id_(options_.my_server_message.id),
      addr_(options.my_server_message.host,
            options.my_server_message.client_port),
      monitor_(new ConnectionMonitor(options.max_all_connections,
                                     options.max_ip_connections)),
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

  skywalker::Member member;
  for (auto& server_message : options_.all_server_messages) {
    member.id = server_message.id;
    member.host = server_message.host;
    member.port = server_message.paxos_port;
    member.context = std::to_string(server_message.client_port);
    group_options.membership.push_back(member);
  }

  db_.reset(new SaberDB(3));
  db_->set_machine_id(6);

  group_options.checkpoint = db_.get();
  group_options.machines.push_back(db_.get());

  skywalker::Options skywalker_options;
  skywalker_options.my.id = options_.my_server_message.id;
  skywalker_options.my.host = options_.my_server_message.host;
  skywalker_options.my.port = options_.my_server_message.paxos_port;

  for (int i = 0; i < options_.paxos_group_size; ++i) {
    group_options.group_id = i;
    skywalker_options.groups.push_back(group_options);
  }

  skywalker::Node* node;
  bool res = skywalker::Node::Start(skywalker_options, &node);
  if (res) {
    LOG_INFO("Skywalker start successful!");
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
  bool result = monitor_->OnConnection(p);
  if (result) {
    // FIXME check session id is unique or not?
    ServerConnection* conn = new ServerConnection(
        GetNextSessionId(), p, db_.get(), node_.get());
    p->SetContext(conn);
    p->SetMessageCallback(
        [conn](const voyager::TcpConnectionPtr& ptr, voyager::Buffer* buf) {
      conn->OnMessage(ptr, buf);
    });
  }
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  monitor_->OnClose(p);
  delete reinterpret_cast<ServerConnection*>(p->Context());
}

uint64_t SaberServer::GetNextSessionId() const {
  static SequencenNumber<int> seq_num_(1 << 10);
  return (NowMillis() << 22) | (server_id_ << 10) | seq_num_.GetNext();
}

}  // namespace saber
