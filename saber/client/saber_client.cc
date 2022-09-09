// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <utility>

#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

static std::string GetRoot(const std::string& path) {
  size_t i = 0;
  for (i = 1; i < path.size(); ++i) {
    if (path[i] == '/') {
      break;
    }
  }
  assert(i > 1);
  return path.substr(0, i);
}

void SaberClient::WeakCallback(std::weak_ptr<SaberClient> client_wp,
                               const voyager::TcpConnectionPtr& p) {
  std::shared_ptr<SaberClient> client = client_wp.lock();
  if (client) {
    client->OnClose(p);
  }
}

SaberClient::SaberClient(voyager::EventLoop* loop, const ClientOptions& options)
    : kRoot(options.root),
      has_started_(false),
      state_(SS_DISCONNECTED),
      can_send_(false),
      message_id_(0),
      session_id_(0),
      retry_time_(50),
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
    LOG_FATAL("Don't forget to call the Close() function.");
  }
  delete server_manager_impl_;
}

void SaberClient::Connect() {
  bool expected = false;
  if (has_started_.compare_exchange_strong(expected, true)) {
    session_id_ = 0;
    ClearMessage();
    Connect(server_manager_->GetNext());
  }
}

void SaberClient::Close() {
  bool expected = true;
  if (has_started_.compare_exchange_strong(expected, false)) {
    loop_->RunInLoop(std::bind(&SaberClient::CloseInLoop, shared_from_this()));
  }
}

void SaberClient::CloseInLoop() {
  assert(client_);
  voyager::TcpConnectionPtr p = client_->GetTcpConnectionPtr();
  if (p) {
    // FIXME
    p->StopRead();
    p->SetCloseCallback(std::bind(
        &SaberClient::WeakCallback,
        std::weak_ptr<SaberClient>(shared_from_this()), std::placeholders::_1));
    SaberMessage message;
    message.set_type(MT_CLOSE);
    codec_.SendMessage(p, message);
    loop_->RemoveTimer(timer_);
  } else {
    loop_->RemoveTimer(delay_);
  }
  client_->Close();
}

bool SaberClient::Create(const CreateRequest& request, void* context,
                         const CreateCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_CREATE);
  message->set_data(request.SerializeAsString());

  CreateRequestT* r = new CreateRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    create_queue_.push_back(std::unique_ptr<CreateRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::Delete(const DeleteRequest& request, void* context,
                         const DeleteCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());

  DeleteRequestT* r = new DeleteRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    delete_queue_.push_back(std::unique_ptr<DeleteRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());

  ExistsRequestT* r = new ExistsRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    exists_queue_.push_back(std::unique_ptr<ExistsRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());

  GetDataRequestT* r =
      new GetDataRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    get_data_queue_.push_back(std::unique_ptr<GetDataRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::SetData(const SetDataRequest& request, void* context,
                          const SetDataCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());

  SetDataRequestT* r =
      new SetDataRequestT(request.path(), nullptr, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    set_data_queue_.push_back(std::unique_ptr<SetDataRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

bool SaberClient::GetChildren(const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const GetChildrenCallback& cb) {
  if (GetRoot(request.path()) != kRoot) {
    LOG_ERROR("error request path %s", request.path().c_str());
    return false;
  }
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());

  GetChildrenRequestT* r =
      new GetChildrenRequestT(request.path(), watcher, context, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(message_id_);
    children_queue_.push_back(std::unique_ptr<GetChildrenRequestT>(r));
    TrySendInLoop(message);
  });
  return true;
}

void SaberClient::Connect(const voyager::SockAddr& addr) {
  if (!has_started_) {
    return;
  }
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
  ConnectRequest request;
  request.set_session_id(session_id_);
  SaberMessage message;
  message.set_id(message_id_++);
  message.set_type(MT_CONNECT);
  message.set_data(request.SerializeAsString());
  message.set_extra_data(kRoot);
  codec_.SendMessage(p, message);
  server_manager_->OnConnection();
  if (state_ != SS_CONNECTING) {
    state_ = SS_CONNECTING;
    TriggerState();
  }
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed!");
  can_send_ = false;
  master_.Clear();
  if (state_ != SS_DISCONNECTED) {
    state_ = SS_DISCONNECTED;
    TriggerState();
  }
  retry_time_ *= 2;
  retry_time_ = retry_time_ < kMaxRetryTime ? retry_time_ : kMaxRetryTime;
  loop_->RemoveTimer(delay_);
  delay_ = loop_->RunAfter(
      retry_time_, [this]() { Connect(server_manager_->GetNext()); });
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close! master %s",
            master_.ShortDebugString().c_str());
  can_send_ = false;
  loop_->RemoveTimer(timer_);
  loop_->RemoveTimer(delay_);
  if (state_ != SS_DISCONNECTED) {
    state_ = SS_DISCONNECTED;
    TriggerState();
  }
  if (master_.host().empty()) {
    retry_time_ *= 2;
    retry_time_ = retry_time_ < kMaxRetryTime ? retry_time_ : kMaxRetryTime;
    delay_ = loop_->RunAfter(
        retry_time_, [this]() { Connect(server_manager_->GetNext()); });
  } else {
    Connect(voyager::SockAddr(master_.host(), (uint16_t)master_.port()));
  }
}

bool SaberClient::OnMessage(const voyager::TcpConnectionPtr& p,
                            std::unique_ptr<SaberMessage> message) {
  bool done = true;
  bool result = true;
  MessageType type = message->type();
  switch (type) {
    case MT_NOTIFICATION:
      done = false;
      OnNotification(message.get());
      break;
    case MT_CREATE:
      result = OnCreate(message.get());
      break;
    case MT_DELETE:
      result = OnDelete(message.get());
      break;
    case MT_EXISTS:
      result = OnExists(message.get());
      break;
    case MT_GETDATA:
      result = OnGetData(message.get());
      break;
    case MT_SETDATA:
      result = OnSetData(message.get());
      break;
    case MT_GETCHILDREN:
      result = OnGetChildren(message.get());
      break;
    case MT_MASTER: {
      done = false;
      master_.Clear();
      master_.ParseFromString(message->data());
      LOG_DEBUG("The master is %s.", master_.ShortDebugString().c_str());
      client_->Close();
      break;
    }
    case MT_PING:
      done = false;
      break;
    case MT_CONNECT:
      done = false;
      OnConnect(message.get());
      break;
    case MT_CLOSE:
      done = false;
      break;
    case MT_SERVERS:
      done = false;
      server_manager_->UpdateServers(message->data());
      break;
    default: {
      assert(false);
      done = false;
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  if (!result) {
    if (outgoing_queue_.empty()) {
      LOG_WARN("Invalid message, type:%d, id:%d, but outgoing_queue_ is empty.",
               type, message->id());
    } else {
      LOG_WARN(
          "Invalid message, type:%d, id:%d, but the front of outgoing_queue_ "
          "type:%d, id: %d.",
          type, message->id(), outgoing_queue_.front()->type(),
          outgoing_queue_.front()->id());
    }
  }
  if (done) {
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
  if (response.code() == RC_OK || response.code() == RC_RECONNECT) {
    retry_time_ = 50;
    // FIXME
    if (session_id_ != 0 && session_id_ != response.session_id()) {
      state_ = SS_EXPIRED;
      TriggerState();
    }
    state_ = SS_CONNECTED;
    TriggerState();
    auto p = client_->GetTcpConnectionPtr();
    for (auto& i : outgoing_queue_) {
      codec_.SendMessage(p, *i);
    }
    can_send_ = true;
    uint64_t timeout = response.timeout();
    timeout = (timeout < 12000 ? (timeout * 4 / 5) : (timeout - 3000));
    timer_ = loop_->RunEvery(timeout, std::bind(&SaberClient::OnTimer, this));
  } else {
    if (session_id_ != 0) {
      state_ = SS_EXPIRED;
      TriggerState();
    }
    session_id_ = 0;
    OnConnection(client_->GetTcpConnectionPtr());
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
  create_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (create_queue_.empty()) {
      return false;
    }
    request = std::move(create_queue_.front());
    create_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  return true;
}

bool SaberClient::OnDelete(SaberMessage* message) {
  if (delete_queue_.empty()) {
    return false;
  }
  DeleteResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(delete_queue_.front());
  delete_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (delete_queue_.empty()) {
      return false;
    }
    request = std::move(delete_queue_.front());
    delete_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  return true;
}

bool SaberClient::OnExists(SaberMessage* message) {
  if (exists_queue_.empty()) {
    return false;
  }
  ExistsResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(exists_queue_.front());
  exists_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (exists_queue_.empty()) {
      return false;
    }
    request = std::move(exists_queue_.front());
    exists_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  if (request->watcher) {
    if (response.code() == RC_OK) {
      watch_manager_.AddDataWatch(request->path, request->watcher);
    } else if (response.code() == RC_NO_NODE) {
      watch_manager_.AddExistsWatch(request->path, request->watcher);
    }
  }
  return true;
}

bool SaberClient::OnGetData(SaberMessage* message) {
  if (get_data_queue_.empty()) {
    return false;
  }
  GetDataResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(get_data_queue_.front());
  get_data_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (get_data_queue_.empty()) {
      return false;
    }
    request = std::move(get_data_queue_.front());
    get_data_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  if (request->watcher && response.code() == RC_OK) {
    watch_manager_.AddDataWatch(request->path, request->watcher);
  }
  request->callback(request->path, request->context, response);
  return true;
}

bool SaberClient::OnSetData(SaberMessage* message) {
  if (set_data_queue_.empty()) {
    return false;
  }
  SetDataResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(set_data_queue_.front());
  set_data_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (set_data_queue_.empty()) {
      return false;
    }
    request = std::move(set_data_queue_.front());
    set_data_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  request->callback(request->path, request->context, response);
  return true;
}

bool SaberClient::OnGetChildren(SaberMessage* message) {
  if (children_queue_.empty()) {
    return false;
  }
  GetChildrenResponse response;
  response.set_code(RC_UNKNOWN);
  auto request = std::move(children_queue_.front());
  children_queue_.pop_front();
  assert(message->id() == request->message_id);
  while (message->id() > request->message_id) {
    request->callback(request->path, request->context, response);
    if (children_queue_.empty()) {
      return false;
    }
    request = std::move(children_queue_.front());
    children_queue_.pop_front();
  }
  if (message->id() != request->message_id) {
    return false;
  }
  response.ParseFromString(message->data());
  if (request->watcher && response.code() == RC_OK) {
    watch_manager_.AddChildWatch(request->path, request->watcher);
  }
  request->callback(request->path, request->context, response);
  return true;
}

void SaberClient::TriggerState() {
  WatchedEvent event;
  event.set_type(ET_NONE);
  event.set_state(state_);
  TriggerWatchers(event);
}

void SaberClient::TriggerWatchers(const WatchedEvent& event) {
  WatcherSetPtr watchers = watch_manager_.Trigger(event);
  if (watchers) {
    for (auto& it : *(watchers.get())) {
      it->Process(event);
    }
  }
}

void SaberClient::ClearMessage() {
  create_queue_.clear();
  delete_queue_.clear();
  exists_queue_.clear();
  get_data_queue_.clear();
  set_data_queue_.clear();
  children_queue_.clear();
  outgoing_queue_.clear();
}

}  // namespace saber
