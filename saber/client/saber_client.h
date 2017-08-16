// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include <deque>
#include <memory>
#include <queue>
#include <string>

#include <voyager/core/bg_eventloop.h>
#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_client.h>

#include "saber/client/callbacks.h"
#include "saber/client/client_options.h"
#include "saber/client/request.h"
#include "saber/client/server_manager.h"
#include "saber/service/watcher.h"
#include "saber/util/runloop_thread.h"

namespace saber {

class Messager;
class ClientWatchManager;

class SaberClient {
 public:
  SaberClient(const ClientOptions& options, Watcher* watcher,
              voyager::EventLoop* send_loop, RunLoop* event_loop);
  ~SaberClient();

  void Start();
  void Stop();

  void Create(const std::string& root, const CreateRequest& request,
              void* context, const CreateCallback& cb);

  void Delete(const std::string& root, const DeleteRequest& request,
              void* context, const DeleteCallback& cb);

  void Exists(const std::string& root, const ExistsRequest& request,
              Watcher* watcher, void* context, const ExistsCallback& cb);

  void GetData(const std::string& root, const GetDataRequest& request,
               Watcher* watcher, void* context, const GetDataCallback& cb);

  void SetData(const std::string& root, const SetDataRequest& request,
               void* context, const SetDataCallback& cb);

  void GetACL(const std::string& root, const GetACLRequest& request,
              void* context, const GetACLCallback& cb);

  void SetACL(const std::string& root, const SetACLRequest& request,
              void* context, const SetACLCallback& cb);

  void GetChildren(const std::string& root, const GetChildrenRequest& request,
                   Watcher* watcher, void* context,
                   const GetChildrenCallback& cb);

 private:
  void Connect(const voyager::SockAddr& addr);
  void Close();
  void TrySendInLoop(SaberMessage* message);
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  void OnMessage(const voyager::TcpConnectionPtr& p, voyager::Buffer* buf);
  bool HandleMessage(std::unique_ptr<SaberMessage> message);
  void TriggerWatchers(WatchedEvent* event);

  std::atomic<bool> has_started_;

  ServerManager* server_manager_;
  voyager::EventLoop* send_loop_;
  RunLoop* event_loop_;

  uint64_t session_id_;
  uint64_t timeout_;
  std::weak_ptr<voyager::TcpConnection> conn_wp_;

  std::unique_ptr<voyager::TcpClient> client_;
  std::unique_ptr<ClientWatchManager> watch_manager_;

  std::queue<std::unique_ptr<Request<CreateCallback> > > create_queue_;
  std::queue<std::unique_ptr<Request<DeleteCallback> > > delete_queue_;
  std::queue<std::unique_ptr<Request<ExistsCallback> > > exists_queue_;
  std::queue<std::unique_ptr<Request<GetDataCallback> > > get_data_queue_;
  std::queue<std::unique_ptr<Request<SetDataCallback> > > set_data_queue_;
  std::queue<std::unique_ptr<Request<GetACLCallback> > > get_acl_queue_;
  std::queue<std::unique_ptr<Request<SetACLCallback> > > set_acl_queue_;
  std::queue<std::unique_ptr<Request<GetChildrenCallback> > > children_queue_;

  std::deque<std::unique_ptr<SaberMessage> > outgoing_queue_;

  Master master_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
