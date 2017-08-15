// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"
#include "saber/net/messager.h"
#include "saber/server/connection_monitor.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"
#include "saber/util/sequence_number.h"
#include "saber/util/timeops.h"

namespace saber {

struct SaberServer::Context {
  Context(int idx, const EntryPtr& e) : index(idx), entry_wp(e) {}
  int index;
  std::weak_ptr<Entry> entry_wp;
};

struct SaberServer::Entry {
  Entry(SaberServer* owner, const voyager::TcpConnectionPtr& p)
      : owner_(owner), wp(p) {}

  ~Entry() {
    if (conn.unique()) {
      MutexLock(&owner_->mutex_);
      owner_->conns_.erase(conn->session_id());
    }
    voyager::TcpConnectionPtr p = wp.lock();
    if (p) {
      p->ShutDown();
    }
  }

  SaberServer* owner_;
  std::weak_ptr<voyager::TcpConnection> wp;
  std::shared_ptr<saber::ServerConnection> conn;
};

SaberServer::SaberServer(voyager::EventLoop* loop, const ServerOptions& options)
    : options_(options),
      server_id_(options_.my_server_message.id),
      addr_(options.my_server_message.host,
            options.my_server_message.client_port),
      monitor_(new ConnectionMonitor(options.max_all_connections,
                                     options.max_ip_connections)),
      server_(loop, addr_, "SaberServer", options.server_thread_size) {}

SaberServer::~SaberServer() {}

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

  db_.reset(new SaberDB(options_.paxos_group_size));
  db_->set_machine_id(10);

  group_options.checkpoint = db_.get();
  group_options.machines.push_back(db_.get());

  skywalker::Options skywalker_options;
  skywalker_options.my.id = options_.my_server_message.id;
  skywalker_options.my.host = options_.my_server_message.host;
  skywalker_options.my.port = options_.my_server_message.paxos_port;

  for (uint32_t i = 0; i < options_.paxos_group_size; ++i) {
    group_options.group_id = i;
    skywalker_options.groups.push_back(group_options);
  }

  skywalker::Node* node;
  bool res = skywalker::Node::Start(skywalker_options, &node);
  if (res) {
    LOG_INFO("Skywalker start successful!");
    node_.reset(node);

    server_.SetConnectionCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnConnection(p); });
    server_.SetCloseCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnClose(p); });
    server_.SetMessageCallback(
        [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
          OnMessage(p, buf);
        });
    server_.Start();

    idle_ticks_ = options_.max_session_timeout / options_.tick_time;
    assert(idle_ticks_ > 0);

    const std::vector<voyager::EventLoop*>* loops = server_.AllLoops();
    for (auto& loop : *loops) {
      loop->RunEvery(1000 * options_.tick_time,
                     std::bind(&SaberServer::OnTimer, this));
      buckets_.insert(
          std::make_pair(loop, std::make_pair(BucketList(idle_ticks_), 0)));
    }
  } else {
    LOG_ERROR("Skywalker start failed!");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  bool result = monitor_->OnConnection(p);
  if (result) {
    EntryPtr entry(new Entry(this, p));
    auto it = buckets_.find(p->OwnerEventLoop());
    assert(it != buckets_.end());
    it->second.first.at(it->second.second).insert(entry);
    p->SetContext(new Context(it->second.second, entry));
  }
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  monitor_->OnClose(p);
  Context* context = reinterpret_cast<Context*>(p->Context());
  delete context;
}

void SaberServer::OnMessage(const voyager::TcpConnectionPtr& p,
                            voyager::Buffer* buf) {
  Context* context = reinterpret_cast<Context*>(p->Context());
  assert(context);
  EntryPtr entry = (context->entry_wp).lock();
  assert(entry);
  auto it = buckets_.find(p->OwnerEventLoop());
  assert(it != buckets_.end());
  if (context->index != it->second.second) {
    it->second.first.at(it->second.second).insert(entry);
    context->index = it->second.second;
  }

  Messager::OnMessage(p, buf, [this, entry](std::unique_ptr<SaberMessage> s) {
    return HandleMessage(entry, std::move(s));
  });
}

void SaberServer::OnTimer() {
  auto it = buckets_.find(voyager::EventLoop::RunLoop());
  assert(it != buckets_.end());
  if (++(it->second.second) == idle_ticks_) {
    it->second.second = 0;
  }
  it->second.first.at(it->second.second).clear();
}

bool SaberServer::HandleMessage(const EntryPtr& entry,
                                std::unique_ptr<SaberMessage> message) {
  if (message->type() != MT_CONNECT) {
    if (entry->conn) {
      return entry->conn->OnMessage(std::move(message));
    }
    return false;
  }
  ConnectRequest request;
  request.ParseFromString(message->data());
  uint64_t session_id = request.session_id();

  mutex_.Lock();
  auto it = conns_.find(session_id);
  if (it != conns_.end()) {
    entry->conn = it->second.lock();
    if (entry->conn) {
      entry->conn->SetTcpConnection(entry->wp.lock());
    } else {
      entry->conn.reset(new ServerConnection(session_id, entry->wp.lock(),
                                             db_.get(), node_.get()));
      it->second = entry->conn;
    }
  } else {
    entry->conn.reset(new ServerConnection(GetNextSessionId(), entry->wp.lock(),
                                           db_.get(), node_.get()));
    conns_.insert(std::make_pair(entry->conn->session_id(), entry->conn));
  }
  mutex_.UnLock();

  ConnectResponse response;
  response.set_session_id(entry->conn->session_id());
  SaberMessage reply_msg;
  reply_msg.set_type(MT_CONNECT);
  reply_msg.set_data(response.SerializeAsString());
  Messager::SendMessage(entry->wp.lock(), reply_msg);
  return true;
}

uint64_t SaberServer::GetNextSessionId() const {
  static SequencenNumber<int> seq_num_(1 << 10);
  return (NowMillis() << 22) | (server_id_ << 10) | seq_num_.GetNext();
}

}  // namespace saber
