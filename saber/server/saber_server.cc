// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"
#include "saber/server/connection_monitor.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/util/hash.h"
#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"
#include "saber/util/sequence_number.h"
#include "saber/util/timeops.h"

namespace saber {

struct SaberServer::Context {
  explicit Context(const EntryPtr& e) : entry_wp(e) {}
  std::weak_ptr<Entry> entry_wp;
};

struct SaberServer::Entry {
  Entry(SaberServer* owner, const voyager::TcpConnectionPtr& p)
      : owner_(owner), index(-1), wp(p) {}

  ~Entry() {
    voyager::TcpConnectionPtr p = wp.lock();
    if (p) {
      p->ShutDown();
    }
    if (conn) {
      owner_->KillSession(conn);
    }
  }

  SaberServer* owner_;
  int index;
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
      server_(loop, addr_, "SaberServer", options.server_thread_size) {
  codec_.SetMessageCallback(std::bind(&SaberServer::OnMessage, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  codec_.SetErrorCallback(std::bind(&SaberServer::OnError, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
}

SaberServer::~SaberServer() {}

bool SaberServer::Start() {
  db_.reset(new SaberDB(options_));
  db_->set_machine_id(10);

  bool res = db_->Recover();
  if (res) {
    LOG_INFO("Saber database recover successful!");
  } else {
    LOG_ERROR("Saber database recover failed!");
    return false;
  }

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

  group_options.checkpoint = db_.get();
  group_options.machines.push_back(db_.get());

  skywalker::Options skywalker_options;
  skywalker_options.my.id = options_.my_server_message.id;
  skywalker_options.my.host = options_.my_server_message.host;
  skywalker_options.my.port = options_.my_server_message.paxos_port;
  skywalker_options.io_thread_size = options_.paxos_io_thread_size;
  skywalker_options.groups.resize(options_.paxos_group_size, group_options);

  skywalker::Node* node;
  res = skywalker::Node::Start(skywalker_options, &node);
  if (res) {
    LOG_INFO("Skywalker start successful!");
    node_.reset(node);

    server_.SetConnectionCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnConnection(p); });
    server_.SetCloseCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnClose(p); });
    server_.SetMessageCallback(
        [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
          codec_.OnMessage(p, buf);
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
    UpdateBuckets(p, entry);
    p->SetContext(new Context(entry));
  }
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  monitor_->OnClose(p);
  Context* context = reinterpret_cast<Context*>(p->Context());
  delete context;
}

bool SaberServer::OnMessage(const voyager::TcpConnectionPtr& p,
                            std::unique_ptr<SaberMessage> message) {
  Context* context = reinterpret_cast<Context*>(p->Context());
  assert(context);
  EntryPtr entry = (context->entry_wp).lock();
  bool b = false;
  if (entry) {
    b = HandleMessage(entry, std::move(message));
    if (b) {
      UpdateBuckets(p, entry);
    } else {
      p->ShutDown();
    }
  }
  return b;
}

void SaberServer::OnError(const voyager::TcpConnectionPtr& p,
                          voyager::ProtoCodecError code) {
  if (code == voyager::kParseError) {
    p->StopRead();
  }
}

void SaberServer::OnTimer() {
  auto it = buckets_.find(voyager::EventLoop::RunLoop());
  assert(it != buckets_.end());
  if (++(it->second.second) == idle_ticks_) {
    it->second.second = 0;
  }
  it->second.first.at(it->second.second).clear();
}

void SaberServer::UpdateBuckets(const voyager::TcpConnectionPtr& p,
                                const EntryPtr& entry) {
  auto it = buckets_.find(p->OwnerEventLoop());
  assert(it != buckets_.end());
  if (entry->index != it->second.second) {
    it->second.first.at(it->second.second).insert(entry);
    entry->index = it->second.second;
  }
}

bool SaberServer::HandleMessage(const EntryPtr& entry,
                                std::unique_ptr<SaberMessage> message) {
  if (message->type() == MT_CLOSE) {
    if (entry->conn) {
      KillSession(entry->conn);
      entry->conn.reset();
    }
    return false;
  }

  if (message->type() != MT_CONNECT) {
    if (entry->conn) {
      return entry->conn->OnMessage(std::move(message));
    }
    return false;
  }

  // FIXME: check message(MT_CONNECT) is repeated?
  uint32_t group_id = Shard(message->extra_data());
  if (node_->IsMaster(group_id)) {
    return OnConnectRequest(group_id, entry, std::move(message));
  } else {
    skywalker::Member i;
    uint64_t version;
    node_->GetMaster(group_id, &i, &version);
    Master master;
    master.set_host(i.host);
    master.set_port(atoi(i.context.c_str()));
    SaberMessage reply_message;
    reply_message.set_type(MT_MASTER);
    reply_message.set_id(message->id());
    reply_message.set_data(master.SerializeAsString());
    codec_.SendMessage(entry->wp.lock(), reply_message);
    return false;
  }
}

bool SaberServer::OnConnectRequest(uint32_t group_id, const EntryPtr& entry,
                                   std::unique_ptr<SaberMessage> message) {
  ConnectRequest request;
  request.ParseFromString(message->data());
  uint64_t session_id = request.session_id();

  if (session_id != 0) {
    // FIXME: make it reconnect?
    uint64_t version;
    if (db_->FindSession(group_id, session_id, &version)) {
      Transaction txn;
      txn.set_time(version);
      message->set_extra_data(txn.SerializeAsString());
    } else {
      session_id = 0;
    }
  }

  if (session_id == 0) {
    session_id = GetNextSessionId();
  }

  request.set_session_id(session_id);
  message->set_data(request.SerializeAsString());

  SaberMessage* reply = message.release();

  voyager::TcpConnectionPtr p = entry->wp.lock();

  bool b = node_->Propose(
      group_id, db_->machine_id(), reply->SerializeAsString(), reply,
      [this, group_id, session_id, p, entry](
          uint64_t instance_id, const skywalker::Status& s, void* context) {
        if (s.ok()) {
          MutexLock lock(&mutex_);
          auto it = conns_.find(session_id);
          if (it != conns_.end()) {
            entry->conn = it->second.lock();
            assert(entry->conn);
            entry->conn->Connect(p);
          } else {
            entry->conn.reset(new ServerConnection(group_id, session_id, p,
                                                   db_.get(), node_.get()));
            conns_.insert(std::make_pair(session_id, entry->conn));
          }
          entry->conn->set_version(instance_id);
        }
        SaberMessage* r = reinterpret_cast<SaberMessage*>(context);
        OnConnectResponse(s.ok(), entry, std::unique_ptr<SaberMessage>(r));
      });
  if (!b) {
    OnConnectResponse(b, entry, std::unique_ptr<SaberMessage>(reply));
  }
  return b;
}

void SaberServer::OnConnectResponse(bool b, const EntryPtr& entry,
                                    std::unique_ptr<SaberMessage> message) {
  ConnectResponse response;
  if (b) {
    response.set_session_id(entry->conn->session_id());
    response.set_timeout(options_.max_session_timeout);
  } else {
    response.set_code(RC_FAILED);
  }
  message->set_data(response.SerializeAsString());
  codec_.SendMessage(entry->wp.lock(), *(message.get()));
}

void SaberServer::KillSession(const std::shared_ptr<ServerConnection>& conn) {
  MutexLock lock(&mutex_);
  if (conn.unique()) {
    conns_.erase(conn->session_id());
    if (db_->FindSession(conn->group_id(), conn->session_id(),
                         conn->version())) {
      CloseRequest request;
      request.set_session_id(conn->session_id());
      SaberMessage message;
      message.set_type(MT_CLOSE);
      message.set_data(request.SerializeAsString());
      Transaction txn;
      txn.set_time(conn->version());
      message.set_extra_data(txn.SerializeAsString());
      // FIXME check master?
      node_->Propose(
          conn->group_id(), db_->machine_id(), message.SerializeAsString(),
          nullptr,
          [this, conn](uint64_t, const skywalker::Status& status, void*) {
            if (!status.ok()) {
              KillSession(conn);
            }
          });
    }
  }
}

uint64_t SaberServer::GetNextSessionId() const {
  static SequenceNumber<int> seq_num_(1 << 10);
  return (NowMillis() << 22) | (server_id_ << 10) | seq_num_.GetNext();
}

uint32_t SaberServer::Shard(const std::string& s) const {
  if (node_->group_size() == 1) {
    return 0;
  } else {
    return (Hash(s.c_str(), s.size(), 0) %
            static_cast<uint32_t>(node_->group_size()));
  }
}

}  // namespace saber
