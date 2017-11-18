// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <utility>

#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

SaberClient::SaberClient(voyager::EventLoop* loop, const ClientOptions& options)
    : kRoot(options.root),
      has_started_(false),
      can_send_(false),
      message_id_(0),
      session_id_(0),
      loop_(loop),
      server_manager_(options.server_manager),
      server_manager_impl_(nullptr),
      watch_manager_(options.watcher) {
  codec_.SetMessageCallback(std::bind(&SaberClient::OnMessage, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  codec_.SetErrorCallback(std::bind(&SaberClient::OnError, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
  if (!server_manager_) {
    server_manager_impl_ = new ServerManagerImpl();
    server_manager_ = server_manager_impl_;
  }
  server_manager_->UpdateServers(options.servers);
}

SaberClient::~SaberClient() {
  if (has_started_) {
    Close();
  }
  delete server_manager_impl_;
}

void SaberClient::Connect() {
  bool expected = false;
  if (has_started_.compare_exchange_strong(expected, true)) {
    ClearMessage();
    Connect(server_manager_->GetNext());
  } else {
    LOG_WARN("SaberClient has connected, don't call it again!");
  }
}

void SaberClient::Close() {
  bool expected = true;
  if (has_started_.compare_exchange_strong(expected, false)) {
    assert(client_);
    SaberMessage message;
    message.set_type(MT_CLOSE);
    codec_.SendMessage(client_->GetTcpConnectionPtr(), message);
    client_->Close();
  } else {
    LOG_WARN("SaberClient has closed, don't call it again!");
  }
}

bool SaberClient::Create(const CreateRequest& request, void* context,
                         const CreateCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_CREATE);
  message->set_data(request.SerializeAsString());

  CreateRequestT* r = new CreateRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    create_queue_.push_back(std::unique_ptr<CreateRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::Delete(const DeleteRequest& request, void* context,
                         const DeleteCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());

  DeleteRequestT* r = new DeleteRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    delete_queue_.push_back(std::unique_ptr<DeleteRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());

  ExistsRequestT* r = new ExistsRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    exists_queue_.push_back(std::unique_ptr<ExistsRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());

  GetDataRequestT* r =
      new GetDataRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    get_data_queue_.push_back(std::unique_ptr<GetDataRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::SetData(const SetDataRequest& request, void* context,
                          const SetDataCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());

  SetDataRequestT* r =
      new SetDataRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    set_data_queue_.push_back(std::unique_ptr<SetDataRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::GetACL(const GetACLRequest& request, void* context,
                         const GetACLCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETACL);
  message->set_data(request.SerializeAsString());

  GetACLRequestT* r = new GetACLRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    get_acl_queue_.push_back(std::unique_ptr<GetACLRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::SetACL(const SetACLRequest& request, void* context,
                         const SetACLCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETACL);
  message->set_data(request.SerializeAsString());

  SetACLRequestT* r = new SetACLRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    set_acl_queue_.push_back(std::unique_ptr<SetACLRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::GetChildren(const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const GetChildrenCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());

  GetChildrenRequestT* r =
      new GetChildrenRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    children_queue_.push_back(std::unique_ptr<GetChildrenRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

void SaberClient::Connect(const voyager::SockAddr& addr) {
  client_.reset(new voyager::TcpClient(loop_, addr, "SaberClient"));
  client_->SetConnectionCallback(
      [this](const voyager::TcpConnectionPtr& p) { OnConnection(p); });
  client_->SetConnectFailureCallback([this]() { OnFailue(); });
  client_->SetCloseCallback(
      [this](const voyager::TcpConnectionPtr& p) { OnClose(p); });
  client_->SetMessageCallback(
      [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
        codec_.OnMessage(p, buf);
      });
  client_->Connect(false);
}

void SaberClient::TrySendInLoop(SaberMessage* message) {
  outgoing_queue_.push_back(std::unique_ptr<SaberMessage>(message));
  if (can_send_) {
    codec_.SendMessage(client_->GetTcpConnectionPtr(), *message);
  }
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!");
  can_send_ = false;
  ConnectRequest request;
  request.set_session_id(session_id_);
  SaberMessage message;
  message.set_id(message_id_++);
  message.set_type(MT_CONNECT);
  message.set_data(request.SerializeAsString());
  message.set_extra_data(kRoot);
  codec_.SendMessage(p, message);
  server_manager_->OnConnection();
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed!");
  if (has_started_) {
    Connect(server_manager_->GetNext());
  }
  master_.clear_host();
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!");
  if (has_started_) {
    if (master_.host().empty() == false) {
      voyager::SockAddr addr(master_.host(),
                             static_cast<uint16_t>(master_.port()));
      Connect(addr);
    } else {
      SleepForMicroseconds(100000);
      Connect(server_manager_->GetNext());
    }
  } else {
    WatchedEvent event;
    event.set_type(ET_NONE);
    event.set_state(SS_DISCONNECTED);
    TriggerWatchers(event);
    session_id_ = 0;
    ClearMessage();
  }
  loop_->RemoveTimer(timer_);
}

bool SaberClient::OnMessage(const voyager::TcpConnectionPtr& p,
                            std::unique_ptr<SaberMessage> message) {
  LOG_DEBUG("type: %d, id: %d, max id:%d, queue size:%d.", message->type(),
            (int)(message->id()), (int)message_id_,
            (int)outgoing_queue_.size());
  MessageType type = message->type();
  switch (type) {
    case MT_NOTIFICATION:
      OnNotification(message.get());
      break;
    case MT_CONNECT:
      OnConnect(message.get());
      break;
    case MT_CREATE:
      OnCreate(message.get());
      break;
    case MT_DELETE:
      OnDelete(message.get());
      break;
    case MT_EXISTS:
      OnExists(message.get());
      break;
    case MT_GETDATA:
      OnGetData(message.get());
      break;
    case MT_SETDATA:
      OnSetData(message.get());
      break;
    case MT_GETACL:
      OnGetACL(message.get());
      break;
    case MT_SETACL:
      OnSetACL(message.get());
      break;
    case MT_GETCHILDREN:
      OnGetChildren(message.get());
      break;
    case MT_MASTER: {
      master_.ParseFromString(message->data());
      LOG_DEBUG("The master is %s:%d.", master_.host().c_str(), master_.port());
      client_->Close();
      break;
    }
    case MT_SERVERS:
      server_manager_->UpdateServers(message->data());
      break;
    case MT_PING:
      break;
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  if (type != MT_NOTIFICATION && type != MT_MASTER && type != MT_PING &&
      type != MT_CONNECT && type != MT_SERVERS) {
    assert(!outgoing_queue_.empty());
    assert(outgoing_queue_.front()->id() == message->id());
    while (!outgoing_queue_.empty() &&
           message->id() >= outgoing_queue_.front()->id()) {
      outgoing_queue_.pop_front();
    }
  }
  return type == MT_MASTER ? false : true;
}

void SaberClient::OnError(const voyager::TcpConnectionPtr& p,
                          voyager::ProtoCodecError code) {
  if (code == voyager::kParseError) {
    p->ForceClose();
  }
}

void SaberClient::OnTimer() {
  SaberMessage message;
  message.set_type(MT_PING);
  codec_.SendMessage(client_->GetTcpConnectionPtr(), message);
}

void SaberClient::OnNotification(SaberMessage* message) {
  WatchedEvent event;
  event.ParseFromString(message->data());
  TriggerWatchers(event);
}

void SaberClient::OnConnect(SaberMessage* message) {
  ConnectResponse response;
  response.ParseFromString(message->data());
  WatchedEvent event;
  if (response.code() == RC_OK || response.code() == RC_RECONNECT) {
    if (session_id_ == 0 || response.session_id() == session_id_) {
      event.set_state(SS_CONNECTED);
    } else {
      event.set_state(SS_EXPIRED);
    }
    for (auto& i : outgoing_queue_) {
      codec_.SendMessage(client_->GetTcpConnectionPtr(), *i);
    }
    can_send_ = true;
    uint64_t timeout = response.timeout();
    timeout = (timeout < 12000000 ? (timeout * 4 / 5) : (timeout - 3000000));
    timer_ = loop_->RunEvery(timeout, std::bind(&SaberClient::OnTimer, this));
  } else {
    if (response.code() == RC_NO_AUTH) {
      event.set_state(SS_AUTHFAILED);
      has_started_ = false;
      client_->Close();
    } else {
      if (session_id_ != 0) {
        event.set_state(SS_EXPIRED);
      }
      session_id_ = 0;
      OnConnection(client_->GetTcpConnectionPtr());
    }
  }
  if (response.code() != RC_RECONNECT) {
    event.set_type(ET_NONE);
    TriggerWatchers(event);
  }
  session_id_ = response.session_id();
}

bool SaberClient::OnCreate(SaberMessage* message) {
  if (create_queue_.empty()) {
    return false;
  }
  CreateResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(create_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    create_queue_.pop_front();
    if (create_queue_.empty()) {
      return false;
    }
    request = std::move(create_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  create_queue_.pop_front();
  return true;
}

bool SaberClient::OnDelete(SaberMessage* message) {
  if (delete_queue_.empty()) {
    return false;
  }
  DeleteResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(delete_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    delete_queue_.pop_front();
    if (delete_queue_.empty()) {
      return false;
    }
    request = std::move(delete_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  delete_queue_.pop_front();
  return true;
}

bool SaberClient::OnExists(SaberMessage* message) {
  if (exists_queue_.empty()) {
    return false;
  }
  ExistsResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(exists_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    exists_queue_.pop_front();
    if (exists_queue_.empty()) {
      return false;
    }
    request = std::move(exists_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  if (request->watcher) {
    if (response.code() == RC_OK) {
      watch_manager_.AddDataWatch(request->path, request->watcher);
    } else {
      watch_manager_.AddExistsWatch(request->path, request->watcher);
    }
  }
  exists_queue_.pop_front();
  return true;
}

bool SaberClient::OnGetData(SaberMessage* message) {
  if (get_data_queue_.empty()) {
    return false;
  }
  GetDataResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(get_data_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    get_data_queue_.pop_front();
    if (get_data_queue_.empty()) {
      return false;
    }
    request = std::move(get_data_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  if (request->watcher && response.code() == RC_OK) {
    watch_manager_.AddDataWatch(request->path, request->watcher);
  }
  request->callback(request->path, request->context, response);
  get_data_queue_.pop_front();
  return true;
}

bool SaberClient::OnSetData(SaberMessage* message) {
  if (set_data_queue_.empty()) {
    return false;
  }
  SetDataResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(set_data_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    set_data_queue_.pop_front();
    if (set_data_queue_.empty()) {
      return false;
    }
    request = std::move(set_data_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  set_data_queue_.pop_front();

  return true;
}

bool SaberClient::OnGetACL(SaberMessage* message) {
  if (get_acl_queue_.empty()) {
    return false;
  }
  GetACLResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(get_acl_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    get_acl_queue_.pop_front();
    if (get_acl_queue_.empty()) {
      return false;
    }
    request = std::move(get_acl_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  get_acl_queue_.pop_front();

  return true;
}

bool SaberClient::OnSetACL(SaberMessage* message) {
  if (set_acl_queue_.empty()) {
    return false;
  }
  SetACLResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(set_acl_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    set_acl_queue_.pop_front();
    if (set_acl_queue_.empty()) {
      return false;
    }
    request = std::move(set_acl_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  set_acl_queue_.pop_front();
  return true;
}

bool SaberClient::OnGetChildren(SaberMessage* message) {
  if (children_queue_.empty()) {
    return false;
  }
  GetChildrenResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(children_queue_.front());
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    children_queue_.pop_front();
    if (children_queue_.empty()) {
      return false;
    }
    request = std::move(children_queue_.front());
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  if (request->watcher && response.code() == RC_OK) {
    watch_manager_.AddChildWatch(request->path, request->watcher);
  }
  request->callback(request->path, request->context, response);
  children_queue_.pop_front();

  return true;
}

void SaberClient::TriggerWatchers(const WatchedEvent& event) {
  WatcherSetPtr watchers = watch_manager_.Trigger(event);
  if (watchers) {
    for (auto& it : *(watchers.get())) {
      it->Process(event);
    }
  }
}

std::string SaberClient::GetRoot(const std::string& path) const {
  size_t i = 0;
  for (i = 1; i < path.size(); ++i) {
    if (path[i] == '/') {
      break;
    }
  }
  assert(i > 1);
  return path.substr(0, i);
}

void SaberClient::ClearMessage() {
  create_queue_.clear();
  delete_queue_.clear();
  exists_queue_.clear();
  get_data_queue_.clear();
  set_data_queue_.clear();
  get_acl_queue_.clear();
  set_acl_queue_.clear();
  children_queue_.clear();
  outgoing_queue_.clear();
}

}  // namespace saber
