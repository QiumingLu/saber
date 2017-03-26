// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <voyager/util/string_util.h>

#include "saber/client/server_manager_impl.h"
#include "saber/util/logging.h"

namespace saber {

SaberClient::SaberClient(voyager::EventLoop* loop,
                         const std::string& servers,
                         std::unique_ptr<ServerManager> manager)
    : loop_(loop),
      server_manager_(std::move(manager)),
      messager_(new Messager()),
      has_started_(false) {
  if (!server_manager_) {
    server_manager_.reset(new ServerManagerImpl());
  }
  server_manager_->UpdateServers(servers);
  messager_->SetMessageCallback([this](std::unique_ptr<SaberMessage> message) {
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

bool SaberClient::Create(const CreateRequest& request, NodeType type,
                         void* context, const CreateCallback& cb) {
  SaberMessage message;
  message.set_type(MT_CREATE);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    create_cb_.push(
        Request<CreateCallback>(request.path(), context, nullptr, cb));
  }
  return res;
}

bool SaberClient::Delete(const DeleteRequest& request,
                         void* context, const DeleteCallback& cb) {
  SaberMessage message;
  message.set_type(MT_DELETE);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    delete_cb_.push(
        Request<DeleteCallback>(request.path(), context, nullptr, cb));
  }
  return res;
}

bool SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  SaberMessage message;
  message.set_type(MT_EXISTS);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    exists_cb_.push(
        Request<ExistsCallback>(request.path(), context, watcher, cb));
  }
  return res;
}

bool SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  SaberMessage message;
  message.set_type(MT_GETDATA);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    get_data_cb_.push(
        Request<GetDataCallback>(request.path(), context, watcher, cb));
  }
  return res;
}

bool SaberClient::SetData(const SetDataRequest& request,
                          void* context, const SetDataCallback& cb) {
  SaberMessage message;
  message.set_type(MT_SETDATA);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    set_data_cb_.push(
        Request<SetDataCallback>(request.path(), context, nullptr, cb));
  }
  return res;
}

bool SaberClient::GetACL(const GetACLRequest& request,
                         void* context, const GetACLCallback& cb) {
  SaberMessage message;
  message.set_type(MT_GETACL);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    get_acl_cb_.push(
        Request<GetACLCallback>(request.path(), context, nullptr, cb));
  }
  return res;
}

bool SaberClient::SetACL(const SetACLRequest& request,
                         void* context, const SetACLCallback& cb) {
  SaberMessage message;
  message.set_type(MT_SETACL);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    set_acl_cb_.push
        (Request<SetACLCallback>(request.path(), context, nullptr, cb));
  }
  return res;
}

bool SaberClient::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                              void* context, const ChildrenCallback& cb) {
  SaberMessage message;
  message.set_type(MT_GETCHILDREN);
  message.set_data(request.SerializeAsString());
  bool res = messager_->SendMessage(message);
  if (res) {
    children_cb_.push(
        Request<ChildrenCallback>(request.path(), context, watcher, cb));
  }
  return res;
}

void SaberClient::Connect() {
  SaberClientPtr my(shared_from_this());

  std::pair<std::string, uint16_t> s = server_manager_->GetNext();
  voyager::SockAddr addr(s.first, s.second);
  client_.reset(new voyager::TcpClient(loop_, addr, "SaberClient"));
  client_->SetConnectionCallback(
      [my](const voyager::TcpConnectionPtr& p) {
    my->OnConnection(p);
  });
  client_->SetConnectFailureCallback([my]() {
    my->OnFailue();
  });
  client_->SetCloseCallback(
      [my](const voyager::TcpConnectionPtr& p) {
    my->OnClose(p);
  });
  client_->SetMessageCallback(
      [my](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
    my->messager_->OnMessage(p, buf);
  });
  client_->Connect(false);
}

void SaberClient::Close() {
  SaberClientPtr my(shared_from_this());
  loop_->RunInLoop([my]() {
    assert(my->client_);
    my->client_->Close();
  });
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!\n");
  messager_->SetTcpConnection(p);
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed, try to next server!\n");
  if (has_started_) {
    Connect();
  }
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!\n");
}

void SaberClient::OnMessage(std::unique_ptr<SaberMessage> message) {
  assert(message);
  switch (message->type()) {
    case MT_NOTIFICATION: {
      WatchedEvent event;
      event.ParseFromString(message->data());
      break;
    }
    case MT_CREATE: {
      CreateResponse response;
      response.ParseFromString(message->data());
      Request<CreateCallback> r(create_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_DELETE: {
      Request<DeleteCallback> r(delete_cb_.take());
      r.cb_(0, r.path_, r.context_);
      break;
    }
    case MT_EXISTS: {
      ExistsResponse response;
      response.ParseFromString(message->data());
      Request<ExistsCallback> r(exists_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_GETDATA: {
      GetDataResponse response;
      response.ParseFromString(message->data());
      Request<GetDataCallback> r(get_data_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_SETDATA: {
      SetDataResponse response;
      response.ParseFromString(message->data());
      Request<SetDataCallback> r(set_data_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_GETACL: {
      GetACLResponse response;
      response.ParseFromString(message->data());
      Request<GetACLCallback> r(get_acl_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_SETACL: {
      SetACLResponse response;
      response.ParseFromString(message->data());
      Request<SetACLCallback> r(set_acl_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenResponse response;
      response.ParseFromString(message->data());
      Request<ChildrenCallback> r(children_cb_.take());
      r.cb_(0, r.path_, r.context_, response);
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
}

}  // namespace saber
