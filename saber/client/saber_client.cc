// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <set>
#include <utility>

#include "saber/client/client_watch_manager.h"
#include "saber/client/server_manager_impl.h"
#include "saber/util/logging.h"
#include "saber/util/timeops.h"

namespace saber {

SaberClient::SaberClient(voyager::EventLoop* loop, const ClientOptions& options,
                         Watcher* watcher)
    : has_started_(false),
      root_(options.root),
      server_manager_(options.server_manager),
      loop_(loop),
      session_id_(0),
      watch_manager_(new ClientWatchManager(options.auto_watch_reset)),
      message_id_(0) {
  watch_manager_->SetDefaultWatcher(watcher);
  codec_.SetMessageCallback(std::bind(&SaberClient::OnMessage, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  codec_.SetErrorCallback(std::bind(&SaberClient::OnError, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
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
    LOG_WARN("SaberClient has started, don't call it again!");
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

void SaberClient::Create(const CreateRequest& request, void* context,
                         const CreateCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_CREATE);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<CreateCallback>* r =
      new Request<CreateCallback>(request.path(), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    create_queue_.push(std::unique_ptr<Request<CreateCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Delete(const DeleteRequest& request, void* context,
                         const DeleteCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_DELETE);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<DeleteCallback>* r =
      new Request<DeleteCallback>(request.path(), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    delete_queue_.push(std::unique_ptr<Request<DeleteCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::Exists(const ExistsRequest& request, Watcher* watcher,
                         void* context, const ExistsCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_EXISTS);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<ExistsCallback>* r =
      new Request<ExistsCallback>(request.path(), context, watcher, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    exists_queue_.push(std::unique_ptr<Request<ExistsCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetData(const GetDataRequest& request, Watcher* watcher,
                          void* context, const GetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETDATA);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<GetDataCallback>* r =
      new Request<GetDataCallback>(request.path(), context, watcher, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    get_data_queue_.push(std::unique_ptr<Request<GetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetData(const SetDataRequest& request, void* context,
                          const SetDataCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETDATA);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<SetDataCallback>* r =
      new Request<SetDataCallback>(request.path(), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    set_data_queue_.push(std::unique_ptr<Request<SetDataCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetACL(const GetACLRequest& request, void* context,
                         const GetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETACL);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<GetACLCallback>* r =
      new Request<GetACLCallback>(request.path(), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    get_acl_queue_.push(std::unique_ptr<Request<GetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::SetACL(const SetACLRequest& request, void* context,
                         const SetACLCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_SETACL);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<SetACLCallback>* r =
      new Request<SetACLCallback>(request.path(), context, nullptr, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    set_acl_queue_.push(std::unique_ptr<Request<SetACLCallback> >(r));
    TrySendInLoop(message);
  });
}

void SaberClient::GetChildren(const GetChildrenRequest& request,
                              Watcher* watcher, void* context,
                              const GetChildrenCallback& cb) {
  SaberMessage* message = new SaberMessage();
  message->set_type(MT_GETCHILDREN);
  message->set_data(request.SerializeAsString());
  message->set_extra_data(root_);

  Request<GetChildrenCallback>* r =
      new Request<GetChildrenCallback>(request.path(), context, watcher, cb);

  loop_->RunInLoop([this, message, r]() {
    r->message_id = ++message_id_;
    message->set_id(r->message_id);
    children_queue_.push(std::unique_ptr<Request<GetChildrenCallback> >(r));
    TrySendInLoop(message);
  });
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

void SaberClient::Close() {
  loop_->RunInLoop([this]() {
    assert(client_);
    client_->Close();
  });
}

void SaberClient::TrySendInLoop(SaberMessage* message) {
  codec_.SendMessage(conn_wp_.lock(), *message);
  outgoing_queue_.push_back(std::unique_ptr<SaberMessage>(message));
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!");
  conn_wp_ = p;
  server_manager_->OnConnection();
  ConnectRequest request;
  request.set_session_id(session_id_);
  SaberMessage message;
  message.set_id(message_id_++);
  message.set_type(MT_CONNECT);
  message.set_data(request.SerializeAsString());
  message.set_extra_data(root_);
  codec_.SendMessage(p, message);
  for (auto& i : outgoing_queue_) {
    codec_.SendMessage(p, *i);
  }
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
      SleepForMicroseconds(1000);
      Connect(server_manager_->GetNext());
    }
  }
}

bool SaberClient::OnMessage(const voyager::TcpConnectionPtr& p,
                            std::unique_ptr<SaberMessage> message) {
  bool res = true;
  int id = message->id();
  MessageType type = message->type();
  switch (type) {
    case MT_NOTIFICATION: {
      WatchedEvent* event = new WatchedEvent();
      event->ParseFromString(message->data());
      TriggerWatchers(event);
      break;
    }
    case MT_CONNECT: {
      ConnectResponse response;
      response.ParseFromString(message->data());
      session_id_ = response.session_id();
      timeout_ = response.timeout();
      WatchedEvent* event = new WatchedEvent();
      event->set_state(SS_CONNECTED);
      event->set_type(ET_NONE);
      TriggerWatchers(event);
      break;
    }
    case MT_CREATE: {
      if (create_queue_.empty()) {
        return true;
      }
      CreateResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(create_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        create_queue_.pop();
        if (create_queue_.empty()) {
          return true;
        }
        request = std::move(create_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        create_queue_.pop();
      }
      break;
    }
    case MT_DELETE: {
      if (delete_queue_.empty()) {
        return true;
      }
      DeleteResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(delete_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        delete_queue_.pop();
        if (delete_queue_.empty()) {
          return true;
        }
        request = std::move(delete_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        delete_queue_.pop();
      }
      break;
    }
    case MT_EXISTS: {
      if (exists_queue_.empty()) {
        return true;
      }
      ExistsResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(exists_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        exists_queue_.pop();
        if (exists_queue_.empty()) {
          return true;
        }
        request = std::move(exists_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        if (request->watcher) {
          if (response.code() == RC_OK) {
            watch_manager_->AddDataWatch(request->path, request->watcher);
          } else {
            watch_manager_->AddExistWatch(request->path, request->watcher);
          }
        }
        exists_queue_.pop();
      }
      break;
    }
    case MT_GETDATA: {
      if (get_data_queue_.empty()) {
        return true;
      }
      GetDataResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(get_data_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        get_data_queue_.pop();
        if (get_data_queue_.empty()) {
          return true;
        }
        request = std::move(get_data_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        if (request->watcher && response.code() == RC_OK) {
          watch_manager_->AddDataWatch(request->path, request->watcher);
        }
        request->callback(request->path, request->context, response);
        get_data_queue_.pop();
      }
      break;
    }
    case MT_SETDATA: {
      if (set_data_queue_.empty()) {
        return true;
      }
      SetDataResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(set_data_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        set_data_queue_.pop();
        if (set_data_queue_.empty()) {
          return true;
        }
        request = std::move(set_data_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        set_data_queue_.pop();
      }
      break;
    }
    case MT_GETACL: {
      if (get_acl_queue_.empty()) {
        return true;
      }
      GetACLResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(get_acl_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        get_acl_queue_.pop();
        if (get_acl_queue_.empty()) {
          return true;
        }
        request = std::move(get_acl_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        get_acl_queue_.pop();
      }
      break;
    }
    case MT_SETACL: {
      if (set_acl_queue_.empty()) {
        return true;
      }
      SetACLResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(set_acl_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        set_acl_queue_.pop();
        if (set_acl_queue_.empty()) {
          return true;
        }
        request = std::move(set_acl_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        request->callback(request->path, request->context, response);
        set_acl_queue_.pop();
      }
      break;
    }
    case MT_GETCHILDREN: {
      if (children_queue_.empty()) {
        return true;
      }
      GetChildrenResponse response;
      response.set_code(RC_UNKNOWN);
      auto request = std::move(children_queue_.front());
      assert(id == request->message_id);
      while (id > request->message_id) {
        request->callback(request->path, request->context, response);
        children_queue_.pop();
        if (children_queue_.empty()) {
          return true;
        }
        request = std::move(children_queue_.front());
      }
      if (id == request->message_id) {
        response.ParseFromString(message->data());
        if (request->watcher && response.code() == RC_OK) {
          watch_manager_->AddChildWatch(request->path, request->watcher);
        }
        request->callback(request->path, request->context, response);
        children_queue_.pop();
      }
      break;
    }
    case MT_MASTER: {
      res = false;
      master_.ParseFromString(message->data());
      LOG_DEBUG("The master is %s:%d.", master_.host().c_str(), master_.port());
      Close();
      break;
    }
    case MT_PING: {
      break;
    }
    default: {
      assert(false);
      LOG_ERROR("Invalid message type.");
      break;
    }
  }
  if (type != MT_NOTIFICATION && type != MT_MASTER && type != MT_PING &&
      type != MT_CONNECT) {
    assert(!outgoing_queue_.empty());
    assert(outgoing_queue_.front()->id() == id);
    while (!outgoing_queue_.empty() && id <= outgoing_queue_.front()->id()) {
      outgoing_queue_.pop_front();
    }
  }
  return res;
}

void SaberClient::OnError(const voyager::TcpConnectionPtr& p,
                          voyager::ProtoCodecError code) {
  if (code == voyager::kParseError) {
    p->ForceClose();
  }
}

void SaberClient::TriggerWatchers(WatchedEvent* event) {
  WatcherSetPtr result = watch_manager_->Trigger(*event);
  // FIXME
  // Use std::shared_ptr to replace native pointer, which can avoid
  // memory leaks occur?
  std::set<Watcher*>* watchers = result.release();
  loop_->RunInLoop([watchers, event]() {
    if (watchers) {
      for (auto& it : *watchers) {
        it->Process(*event);
      }
      delete watchers;
    }
    delete event;
  });
}

}  // namespace saber
