// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/saber_server.h"
#include "saber/server/saber_db.h"
#include "saber/server/saber_session.h"
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
      : owner_(owner), index(-1), started(false), conn_wp(p) {}

  ~Entry() {
    voyager::TcpConnectionPtr p = conn_wp.lock();
    if (p) {
      p->ShutDown();
    }
    if (session) {
      owner_->CloseSession(session);
    }
  }

  SaberServer* owner_;
  int index;
  std::atomic<bool> started;
  std::weak_ptr<voyager::TcpConnection> conn_wp;
  std::shared_ptr<saber::SaberSession> session;
};

SaberServer::SaberServer(voyager::EventLoop* loop, const ServerOptions& options)
    : options_(options),
      server_id_(options_.my_server_message.id),
      idle_ticks_(options_.session_timeout / options_.tick_time),
      mutexes_(options_.paxos_group_size),
      sessions_(options_.paxos_group_size),
      monitor_(options.max_all_connections, options.max_ip_connections),
      server_(loop, voyager::SockAddr(options.my_server_message.host,
                                      options.my_server_message.client_port),
              "SaberServer", options.server_thread_size) {
  codec_.SetMessageCallback(std::bind(&SaberServer::OnMessage, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  codec_.SetErrorCallback(std::bind(&SaberServer::OnError, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
}

SaberServer::~SaberServer() {}

bool SaberServer::Start() {
  loop_ = thread_.Loop();
  db_.reset(new SaberDB(loop_, options_));
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
  skywalker_options.io_thread_size = options_.paxos_io_thread_size;
  skywalker_options.callback_thread_size = options_.paxos_callback_thread_size;
  skywalker_options.my.id = options_.my_server_message.id;
  skywalker_options.my.host = options_.my_server_message.host;
  skywalker_options.my.port = options_.my_server_message.paxos_port;
  skywalker_options.groups.resize(options_.paxos_group_size, group_options);

  skywalker_options.master_cb = [this](uint32_t group_id) {
    loop_->QueueInLoop(std::bind(&SaberServer::CleanSessions, this, group_id));
  };
  skywalker_options.membership_cb = [this](uint32_t group_id) {
    loop_->QueueInLoop(std::bind(&SaberServer::NewServers, this, group_id));
  };

  skywalker::Node* node;
  res = skywalker::Node::Start(skywalker_options, &node);
  if (res) {
    LOG_INFO("Skywalker start successful!");
    node_.reset(node);

    SaberSession::kMaxDataSize = options_.max_data_size;
    for (uint32_t i = 0; i < options_.paxos_group_size; ++i) {
      loop_->QueueInLoop(std::bind(&SaberServer::CleanSessions, this, i));
    }

    server_.SetConnectionCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnConnection(p); });
    server_.SetCloseCallback(
        [this](const voyager::TcpConnectionPtr& p) { OnClose(p); });
    server_.SetMessageCallback(
        [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
          codec_.OnMessage(p, buf);
        });
    server_.Start();

    const std::vector<voyager::EventLoop*>* loops = server_.AllLoops();
    for (auto& loop : *loops) {
      buckets_.insert(
          std::make_pair(loop, std::make_pair(BucketList(idle_ticks_), 0)));
      loop->RunEvery(options_.tick_time,
                     std::bind(&SaberServer::OnTimer, this));
    }
  } else {
    LOG_ERROR("Skywalker start failed!");
  }
  return res;
}

void SaberServer::OnConnection(const voyager::TcpConnectionPtr& p) {
  bool result = monitor_.OnConnection(p);
  if (result) {
    EntryPtr entry = std::make_shared<Entry>(this, p);
    UpdateBuckets(p, entry);
    p->SetContext(new Context(entry));
  }
}

void SaberServer::OnClose(const voyager::TcpConnectionPtr& p) {
  monitor_.OnClose(p);
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
  LOG_WARN("proto codec error, the code is %d", code);
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
    it->second.first.at(it->second.second).push_back(entry);
    entry->index = it->second.second;
  }
}

bool SaberServer::HandleMessage(const EntryPtr& entry,
                                std::unique_ptr<SaberMessage> message) {
  if (message->type() != MT_CONNECT) {
    if (entry->session) {
      return entry->session->OnMessage(std::move(message));
    }
    // FIXME only ignore the message?
    return false;
  }

  std::string root = message->extra_data();
  message->clear_extra_data();
  uint32_t group_id = Shard(root);
  if (node_->IsMaster(group_id)) {
    return OnConnectRequest(root, group_id, entry, std::move(message));
  } else {
    skywalker::Member i;
    uint64_t version;
    node_->GetMaster(group_id, &i, &version);
    Master master;
    master.set_host(i.host);
    master.set_port(atoi(i.context.c_str()));
    message->set_type(MT_MASTER);
    message->set_data(master.SerializeAsString());
    codec_.SendMessage(entry->conn_wp.lock(), *message);
    return false;
  }
}

bool SaberServer::OnConnectRequest(const std::string& root, uint32_t group_id,
                                   const EntryPtr& entry,
                                   std::unique_ptr<SaberMessage> message) {
  // Ignore the repeated request.
  if (entry->started) {
    return true;
  }
  entry->started = true;

  ConnectRequest request;
  ConnectResponse response;

  request.ParseFromString(message->data());
  uint64_t session_id = request.session_id();

  if (session_id != 0) {
    // Check whether the session still exists or has been moved.
    {
      MutexLock lock(&mutexes_[group_id]);
      auto it = sessions_[group_id].find(session_id);
      if (it != sessions_[group_id].end()) {
        std::shared_ptr<SaberSession> session = it->second.lock();
        if (session) {
          voyager::TcpConnectionPtr p = session->GetTcpConnectionPtr();
          if (p && p->IsConnected()) {
            response.set_code(RC_UNKNOWN);
            message->set_data(response.SerializeAsString());
            OnConnectResponse(entry, std::move(message));
            return false;
          }
        }
      }
    }

    uint64_t version;
    if (db_->FindSession(group_id, session_id, &version)) {
      request.set_version(version);
    } else {
      // FIXME Only send back to tell the session has been closed?
      session_id = 0;
    }
  }

  if (session_id == 0) {
    request.set_version(0);
    session_id = GetNextSessionId();
  }

  request.set_session_id(session_id);
  message->set_data(request.SerializeAsString());
  std::string value(message->SerializeAsString());

  SaberMessage* reply = message.release();
  response.set_code(RC_FAILED);
  reply->set_data(response.SerializeAsString());

  bool b = node_->Propose(
      group_id, db_->machine_id(), value, reply,
      [this, root, group_id, session_id, entry](
          uint64_t instance_id, const skywalker::Status& s, void* context) {
        SaberMessage* r = reinterpret_cast<SaberMessage*>(context);
        ConnectResponse res;
        res.ParseFromString(r->data());
        if (res.code() != RC_OK) {
          entry->started = false;
        } else {
          if (CreateSession(root, group_id, session_id, instance_id, entry)) {
            res.set_code(RC_RECONNECT);
          }
          res.set_session_id(session_id);
          res.set_timeout(options_.session_timeout);
          r->set_data(res.SerializeAsString());
        }
        OnConnectResponse(entry, std::unique_ptr<SaberMessage>(r));
        LOG_INFO("Group %u: create session(id=%llu):%s", group_id,
                 (unsigned long long)session_id, s.ToString().c_str());
      });

  if (!b) {
    entry->started = false;
    OnConnectResponse(entry, std::unique_ptr<SaberMessage>(reply));
  }
  return true;
}

void SaberServer::OnConnectResponse(const EntryPtr& entry,
                                    std::unique_ptr<SaberMessage> message) {
  codec_.SendMessage(entry->conn_wp.lock(), *message);
}

void SaberServer::OnCloseRequest(uint32_t group_id,
                                 const CloseRequest& request) {
  SaberMessage message;
  message.set_type(MT_CLOSE);
  message.set_data(request.SerializeAsString());
  node_->Propose(
      group_id, db_->machine_id(), message.SerializeAsString(), nullptr,
      [group_id](uint64_t, const skywalker::Status& s, void*) {
        LOG_INFO("Group %u: close session:%s", group_id, s.ToString().c_str());
      });
}

bool SaberServer::CreateSession(const std::string& root, uint32_t group_id,
                                uint64_t session_id, uint64_t version,
                                const EntryPtr& entry) {
  bool b = true;
  MutexLock lock(&mutexes_[group_id]);
  auto it = sessions_[group_id].find(session_id);
  if (it != sessions_[group_id].end()) {
    entry->session = it->second.lock();
    assert(entry->session);
    entry->session->OnConnection(entry->conn_wp.lock());
  } else {
    b = false;
    entry->session = std::make_shared<SaberSession>(root, group_id, session_id,
                                                    entry->conn_wp.lock(),
                                                    db_.get(), node_.get());
    sessions_[group_id].insert(std::make_pair(session_id, entry->session));
  }
  entry->session->set_version(version);
  return b;
}

void SaberServer::CloseSession(const std::shared_ptr<SaberSession>& session) {
  bool need_kill = false;
  mutexes_[session->group_id()].Lock();
  if (session.unique()) {
    sessions_[session->group_id()].erase(session->session_id());
    need_kill = true;
  }
  mutexes_[session->group_id()].UnLock();

  if (need_kill && node_->IsMaster(session->group_id()) &&
      db_->FindSession(session->group_id(), session->session_id(),
                       session->version())) {
    CloseRequest request;
    request.add_session_id(session->session_id());
    request.add_version(session->version());
    OnCloseRequest(session->group_id(), request);
  }
}

void SaberServer::CleanSessions(uint32_t group_id) {
  if (!node_) {
    return;
  }
  if (!node_->IsMaster(group_id)) {
    MutexLock lock(&mutexes_[group_id]);
    for (auto& it : sessions_[group_id]) {
      auto session = it.second.lock();
      if (session) {
        SaberMessage* message = new SaberMessage();
        message->set_type(MT_MASTER);
        session->OnMessage(std::unique_ptr<SaberMessage>(message));
      }
    }
    sessions_[group_id].clear();
    return;
  }
  auto sessions = db_->CopySessions(group_id);
  if (sessions->empty()) {
    delete sessions;
    return;
  }
  loop_->RunAfter(8000000, [this, group_id, sessions]() {
    CloseRequest request;
    mutexes_[group_id].Lock();
    for (auto& it : *sessions) {
      if (sessions_[group_id].find(it.first) == sessions_[group_id].end()) {
        request.add_session_id(it.first);
        request.add_version(it.second);
      }
    }
    mutexes_[group_id].UnLock();
    if (request.session_id_size() > 0) {
      OnCloseRequest(group_id, request);
    }
    delete sessions;
  });
}

void SaberServer::NewServers(uint32_t group_id) {
  if (!node_) {
    return;
  }
  std::vector<skywalker::Member> members;
  uint64_t version;
  node_->GetMembership(group_id, &members, &version);

  std::string s;
  s.resize(24 * members.size());
  for (auto& i : members) {
    s.append(i.host);
    s.append(":");
    s.append(i.context);
    s.append(",");
  }
  s.pop_back();

  SaberMessage message;
  message.set_type(MT_SERVERS);
  message.set_data(std::move(s));
  message.set_extra_data(std::to_string(version));

  MutexLock lock(&mutexes_[group_id]);
  for (auto& it : sessions_[group_id]) {
    auto session = it.second.lock();
    if (session) {
      codec_.SendMessage(session->GetTcpConnectionPtr(), message);
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
    return (voyager::Hash32(s) % static_cast<uint32_t>(node_->group_size()));
  }
}

}  // namespace saber
