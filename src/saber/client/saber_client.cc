// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"
#include "saber/client/server_manager_impl.h"
#include "saber/client/client_watch_manager.h"
#include "saber/net/messager.h"
#include "saber/util/logging.h"

namespace saber {

SaberClient::SaberClient(const std::string& servers,
                         std::unique_ptr<ServerManager> manager)
    : loop_(thread_.Loop()),
      server_manager_(std::move(manager)),
      messager_(new Messager()),
      watch_manager_(new ClientWatchManager()),
      has_started_(false) {
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

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<CreateCallback>* r = new Request<CreateCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    create_queue_.push(std::unique_ptr<Request<CreateCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Delete(const DeleteRequest& request,
                         void* context, const DeleteCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<DeleteCallback>* r = new Request<DeleteCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    delete_queue_.push(std::unique_ptr<Request<DeleteCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<ExistsCallback>* r = new Request<ExistsCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    exists_queue_.push(std::unique_ptr<Request<ExistsCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<GetDataCallback>* r = new Request<GetDataCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    get_data_queue_.push(std::unique_ptr<Request<GetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetData(const SetDataRequest& request,
                          void* context, const SetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<SetDataCallback>* r = new Request<SetDataCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    set_data_queue_.push(std::unique_ptr<Request<SetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetACL(const GetACLRequest& request,
                         void* context, const GetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETACL);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<GetACLCallback>* r = new Request<GetACLCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    get_acl_queue_.push(std::unique_ptr<Request<GetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetACL(const SetACLRequest& request,
                         void* context, const SetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETACL);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<SetACLCallback>* r = new Request<SetACLCallback>(
      std::move(client_path), std::move(server_path), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    set_acl_queue_.push(std::unique_ptr<Request<SetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetChildren(const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const ChildrenCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());

  std::string client_path = request.path();
  std::string server_path = client_path;
  Request<ChildrenCallback>* r = new Request<ChildrenCallback>(
      std::move(client_path), std::move(server_path), context, watcher, cb);

  loop_->RunInLoop([this, message, r]() {
    children_queue_.push(std::unique_ptr<Request<ChildrenCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Connect() {
  client_.reset(new voyager::TcpClient(loop_, server_manager_->GetNext()));
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

void SaberClient::TrySendInLoop(SaberMessage* message) {
  messager_->SendMessage(*message);
  outgoing_queue_.push_back(std::unique_ptr<SaberMessage>(message));
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!\n");
  messager_->SetTcpConnection(p);
  server_manager_->OnConnection();
  for (auto& i : outgoing_queue_) {
    messager_->SendMessage(*i);
  }
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed!\n");
  if (has_started_) {
    Connect();
  }
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!\n");
}

void SaberClient::OnMessage(std::unique_ptr<SaberMessage> message) {
  assert(!outgoing_queue_.empty());
  outgoing_queue_.pop_front();
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
      r->callback(r->client_path, r->context, response);
      create_queue_.pop();
      break;
    }
    case MT_DELETE: {
      auto& r = delete_queue_.front();
      DeleteResponse response;
      response.ParseFromString(message->data());
      r->callback(r->client_path, r->context, response);
      delete_queue_.pop();
      break;
    }
    case MT_EXISTS: {
      ExistsResponse response;
      response.ParseFromString(message->data());
      auto& r = exists_queue_.front();
      if (r->watcher) {
        if (response.code() == RC_OK) {
          watch_manager_->AddDataWatch(r->client_path, r->watcher);
        } else {
          watch_manager_->AddExistWatch(r->client_path, r->watcher);
        }
      }
      r->callback(r->client_path, r->context, response);
      exists_queue_.pop();
      break;
    }
    case MT_GETDATA: {
      GetDataResponse response;
      response.ParseFromString(message->data());
      auto& r = get_data_queue_.front();
      if (r->watcher && response.code() == RC_OK) {
        watch_manager_->AddDataWatch(r->client_path, r->watcher);
      }
      r->callback(r->client_path, r->context, response);
      get_data_queue_.pop();
      break;
    }
    case MT_SETDATA: {
      SetDataResponse response;
      response.ParseFromString(message->data());
      auto& r = set_data_queue_.front();
      r->callback(r->client_path, r->context, response);
      set_data_queue_.pop();
      break;
    }
    case MT_GETACL: {
      GetACLResponse response;
      response.ParseFromString(message->data());
      auto& r = get_acl_queue_.front();
      r->callback(r->client_path, r->context, response);
      get_acl_queue_.pop();
      break;
    }
    case MT_SETACL: {
      SetACLResponse response;
      response.ParseFromString(message->data());
      auto& r = set_acl_queue_.front();
      r->callback(r->client_path, r->context, response);
      set_acl_queue_.pop();
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenResponse response;
      response.ParseFromString(message->data());
      auto& r = children_queue_.front();
      if (r->watcher && response.code() == RC_OK) {
        watch_manager_->AddChildWatch(r->client_path, r->watcher);
      }
      r->callback(r->client_path, r->context, response);
      children_queue_.pop();
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
}

}  // namespace saber
