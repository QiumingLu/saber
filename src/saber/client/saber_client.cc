// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"
#include "saber/client/server_manager_impl.h"
#include "saber/client/client_watch_manager.h"
#include "saber/net/messager.h"
#include "saber/util/logging.h"

namespace saber {

SaberClient::SaberClient(const Options& options,
                         voyager::EventLoop* send_loop,
                         RunLoop* event_loop)
    : has_started_(false),
      server_manager_(options.server_manager),
      send_loop_(send_loop),
      event_loop_(event_loop),
      messager_(new Messager()),
      watch_manager_(new ClientWatchManager(options.auto_watch_reset)) {
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
}

SaberClient::~SaberClient() {
  if (has_started_) {
    Stop();
  }
}

void SaberClient::Start() {
  bool expected = false;
  if (has_started_.compare_exchange_strong(expected, true)) {
    Connect(server_manager_->GetNext());
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

void SaberClient::Create(const std::string& root,
                         const CreateRequest& request,
                         void* context, const CreateCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_CREATE);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<CreateCallback>* r =
      new Request<CreateCallback>(request.path(), context, nullptr, cb);

  send_loop_->RunInLoop([this, message, r]() {
    create_queue_.push(std::unique_ptr<Request<CreateCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Delete(const std::string& root,
                         const DeleteRequest& request,
                         void* context, const DeleteCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<DeleteCallback>* r =
      new Request<DeleteCallback>(request.path(), context, nullptr, cb);

  send_loop_->RunInLoop([this, message, r]() {
    delete_queue_.push(std::unique_ptr<Request<DeleteCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Exists(const std::string& root,
                         const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<ExistsCallback>* r =
      new Request<ExistsCallback>(request.path(), context, watcher, cb);

  send_loop_->RunInLoop([this, message, r]() {
    exists_queue_.push(std::unique_ptr<Request<ExistsCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetData(const std::string& root,
                          const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<GetDataCallback>* r =
      new Request<GetDataCallback>(request.path(), context, watcher, cb);

  send_loop_->RunInLoop([this, message, r]() {
    get_data_queue_.push(std::unique_ptr<Request<GetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetData(const std::string& root,
                          const SetDataRequest& request,
                          void* context, const SetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<SetDataCallback>* r =
      new Request<SetDataCallback>(request.path(), context, nullptr, cb);

  send_loop_->RunInLoop([this, message, r]() {
    set_data_queue_.push(std::unique_ptr<Request<SetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetACL(const std::string& root,
                         const GetACLRequest& request,
                         void* context, const GetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETACL);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<GetACLCallback>* r =
      new Request<GetACLCallback>(request.path(), context, nullptr, cb);

  send_loop_->RunInLoop([this, message, r]() {
    get_acl_queue_.push(std::unique_ptr<Request<GetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetACL(const std::string& root,
                         const SetACLRequest& request,
                         void* context, const SetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETACL);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<SetACLCallback>* r =
      new Request<SetACLCallback>(request.path(), context, nullptr, cb);

  send_loop_->RunInLoop([this, message, r]() {
    set_acl_queue_.push(std::unique_ptr<Request<SetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetChildren(const std::string& root,
                              const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const GetChildrenCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root);

  Request<GetChildrenCallback>* r =
      new Request<GetChildrenCallback>(request.path(), context, watcher, cb);

  send_loop_->RunInLoop([this, message, r]() {
    children_queue_.push(std::unique_ptr<Request<GetChildrenCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Connect(const voyager::SockAddr& addr) {
  client_.reset( new voyager::TcpClient(send_loop_, addr));
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
  send_loop_->RunInLoop([this]() {
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
    Connect(server_manager_->GetNext());
  }
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!\n");
  if (has_started_) {
    if (!master_.ip().empty()) {
      voyager::SockAddr addr(
          master_.ip(), static_cast<uint16_t>(master_.port()));
      Connect(addr);
      master_.clear_ip();
    } else {
      Connect(server_manager_->GetNext());
    }
  }
}

void SaberClient::OnMessage(std::unique_ptr<SaberMessage> message) {
  MessageType type = message->type();
  switch (type) {
    case MT_NOTIFICATION: {
      WatchedEvent* event = new WatchedEvent();
      event->ParseFromString(message->data());
      WatcherSetPtr result = watch_manager_->Trigger(*event);
      // FIXME
      // Use std::shared_ptr to replace native pointer, which can avoid
      // memory leaks occur?
      std::set<Watcher*>* watchers = result.release();
      event_loop_->RunInLoop([watchers, event]() {
        if (watchers) {
          for (auto& it : *watchers) {
            it->Process(*event);
          }
          delete watchers;
        }
        delete event;
      });
      break;
    }
    case MT_CREATE: {
      CreateResponse* response = new CreateResponse();
      response->ParseFromString(message->data());
      auto& r = create_queue_.front();
      Request<CreateCallback>* request = r.release();
      create_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_DELETE: {
      DeleteResponse* response = new DeleteResponse();
      response->ParseFromString(message->data());
      auto& r = delete_queue_.front();
      Request<DeleteCallback>* request = r.release();
      delete_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_EXISTS: {
      ExistsResponse* response = new ExistsResponse();
      response->ParseFromString(message->data());
      auto& r = exists_queue_.front();
      if (r->watcher) {
        if (response->code() == RC_OK) {
          watch_manager_->AddDataWatch(r->path, r->watcher);
        } else {
          watch_manager_->AddExistWatch(r->path, r->watcher);
        }
      }
      Request<ExistsCallback>* request = r.release();
      exists_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
     break;
    }
    case MT_GETDATA: {
      GetDataResponse* response = new GetDataResponse();
      response->ParseFromString(message->data());
      auto& r = get_data_queue_.front();
      if (r->watcher && response->code() == RC_OK) {
        watch_manager_->AddDataWatch(r->path, r->watcher);
      }
      Request<GetDataCallback>* request = r.release();
      get_data_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_SETDATA: {
      SetDataResponse* response = new SetDataResponse();
      response->ParseFromString(message->data());
      auto& r = set_data_queue_.front();
      Request<SetDataCallback>* request = r.release();
      set_data_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_GETACL: {
      GetACLResponse* response = new GetACLResponse();
      response->ParseFromString(message->data());
      auto& r = get_acl_queue_.front();
      Request<GetACLCallback>* request = r.release();
      get_acl_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_SETACL: {
      SetACLResponse* response = new SetACLResponse();
      response->ParseFromString(message->data());
      auto& r = set_acl_queue_.front();
      Request<SetACLCallback>* request = r.release();
      set_acl_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenResponse* response = new GetChildrenResponse();
      response->ParseFromString(message->data());
      auto& r = children_queue_.front();
      if (r->watcher && response->code() == RC_OK) {
        watch_manager_->AddChildWatch(r->path, r->watcher);
      }
      Request<GetChildrenCallback>* request = r.release();
      children_queue_.pop();
      event_loop_->RunInLoop([request, response]() {
        request->callback(request->path, request->context, *response);
        delete request;
        delete response;
      });
      break;
    }
    case MT_MASTER: {
      master_.ParseFromString(message->data());
      Close();
      break;
    }
    case MT_PING: {
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
  if (type != MT_NOTIFICATION && type != MT_MASTER && type != MT_PING) {
    assert(!outgoing_queue_.empty());
    outgoing_queue_.pop_front();
  }
}

}  // namespace saber
