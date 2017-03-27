// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"
#include "saber/client/server_manager_impl.h"
#include "saber/util/logging.h"

namespace saber {

SaberClient::SaberClient(const std::string& servers,
                         std::unique_ptr<ServerManager> manager)
    : loop_(thread_.Loop()),
      server_manager_(std::move(manager)),
      messager_(new Messager()),
      has_started_(false),
      has_cb_(false) {
  if (!server_manager_) {
    server_manager_.reset(new ServerManagerImpl());
  }
  server_manager_->UpdateServers(servers);
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
}

SaberClient::~SaberClient() {
}

void SaberClient::Start() {
  bool expected = false;
  if (has_started_.compare_exchange_strong(expected, true)) {
    Connect();
  } else {
    LOG_WARN("SaberClient has started, don't call it again!\n");
  }
}

void SaberClient::Stop() {
  bool expected = true;
  if (has_started_.compare_exchange_strong(expected, false)) {
    Close();
  } else {
    LOG_WARN("SaberClient has stoped, don't call it again!");
  }
}

void SaberClient::Create(const CreateRequest& request,
                         void* context, const CreateCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_CREATE);
  message->set_data(request.SerializeAsString());
  Request<CreateCallback>* r =
      new Request<CreateCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    create_queue_.push(std::unique_ptr<Request<CreateCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::Delete(const DeleteRequest& request,
                         void* context, const DeleteCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());
  Request<DeleteCallback>* r =
      new Request<DeleteCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    delete_queue_.push(std::unique_ptr<Request<DeleteCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());
  Request<ExistsCallback>* r =
      new Request<ExistsCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    exists_queue_.push(std::unique_ptr<Request<ExistsCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());
  Request<GetDataCallback>* r =
      new Request<GetDataCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    get_data_queue_.push(std::unique_ptr<Request<GetDataCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::SetData(const SetDataRequest& request,
                          void* context, const SetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());
  Request<SetDataCallback>* r =
      new Request<SetDataCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    set_data_queue_.push(std::unique_ptr<Request<SetDataCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::GetACL(const GetACLRequest& request,
                         void* context, const GetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETACL);
  message->set_data(request.SerializeAsString());
  Request<GetACLCallback>* r =
      new Request<GetACLCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    get_acl_queue_.push(std::unique_ptr<Request<GetACLCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::SetACL(const SetACLRequest& request,
                         void* context, const SetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETACL);
  message->set_data(request.SerializeAsString());
  Request<SetACLCallback>* r =
      new Request<SetACLCallback>(request.path(), context, nullptr, cb);
  loop_->RunInLoop([this, message, r]() {
    set_acl_queue_.push(std::unique_ptr<Request<SetACLCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::GetChildren(const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const ChildrenCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());
  Request<ChildrenCallback>* r =
      new Request<ChildrenCallback>(request.path(), context, watcher, cb);
  loop_->RunInLoop([this, message, r]() {
    children_queue_.push(std::unique_ptr<Request<ChildrenCallback> >(r));
    outgoing_queue_.push(std::unique_ptr<SaberMessage>(message));
    TrySendInLoop();
  });
}

void SaberClient::Connect() {
  std::pair<std::string, uint16_t> s = server_manager_->GetNext();
  voyager::SockAddr addr(s.first, s.second);
  client_.reset(new voyager::TcpClient(loop_, addr, "SaberClient"));
  client_->SetConnectionCallback(
      [this](const voyager::TcpConnectionPtr& p) {
    OnConnection(p);
  });
  client_->SetConnectFailureCallback([this]() {
    OnFailue();
  });
  client_->SetCloseCallback(
      [this](const voyager::TcpConnectionPtr& p) {
    OnClose(p);
  });
  client_->SetMessageCallback(
      [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
    messager_->OnMessage(p, buf);
  });
  client_->Connect(false);
}

void SaberClient::Close() {
  loop_->RunInLoop([this]() {
    assert(client_);
    client_->Close();
  });
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!\n");
  messager_->SetTcpConnection(p);
  has_cb_ = true;
  TrySendInLoop();
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed!\n");
  if (has_started_) {
    Connect();
  }
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!\n");
  has_cb_ = false;
}

void SaberClient::OnMessage(std::unique_ptr<SaberMessage> message) {
  assert(!outgoing_queue_.empty());
  outgoing_queue_.pop();
  has_cb_ = true;
  TrySendInLoop();
  switch (message->type()) {
    case MT_NOTIFICATION: {
      WatchedEvent event;
      event.ParseFromString(message->data());
      break;
    }
    case MT_CREATE: {
      CreateResponse response;
      response.ParseFromString(message->data());
      auto& r = create_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      create_queue_.pop();
      break;
    }
    case MT_DELETE: {
      auto& r = delete_queue_.front();
      r->cb_(0, r->path_, r->context_);
      delete_queue_.pop();
      break;
    }
    case MT_EXISTS: {
      ExistsResponse response;
      response.ParseFromString(message->data());
      auto& r = exists_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      exists_queue_.pop();
      break;
    }
    case MT_GETDATA: {
      GetDataResponse response;
      response.ParseFromString(message->data());
      auto& r = get_data_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      get_data_queue_.pop();
      break;
    }
    case MT_SETDATA: {
      SetDataResponse response;
      response.ParseFromString(message->data());
      auto& r = set_data_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      set_data_queue_.pop();
      break;
    }
    case MT_GETACL: {
      GetACLResponse response;
      response.ParseFromString(message->data());
      auto& r = get_acl_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      get_acl_queue_.pop();
      break;
    }
    case MT_SETACL: {
      SetACLResponse response;
      response.ParseFromString(message->data());
      auto& r = set_acl_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      set_acl_queue_.pop();
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenResponse response;
      response.ParseFromString(message->data());
      auto& r = children_queue_.front();
      r->cb_(0, r->path_, r->context_, response);
      children_queue_.pop();
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
}

void SaberClient::TrySendInLoop() {
  if (has_started_ && has_cb_ && !outgoing_queue_.empty()) {
    messager_->SendMessage(*(outgoing_queue_.front()));
    has_cb_ = false;
  }
}

}  // namespace saber
