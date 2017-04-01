// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include <memory>
#include <queue>
#include <deque>
#include <voyager/core/eventloop.h>
#include <voyager/core/bg_eventloop.h>
#include <voyager/core/tcp_client.h>

#include "saber/client/callbacks.h"
#include "saber/client/server_manager.h"
#include "saber/client/request.h"
#include "saber/client/options.h"
#include "saber/service/watcher.h"
#include "saber/util/runloop_thread.h"

namespace saber {

class Messager;
class ClientWatchManager;

class SaberClient {
 public:
  SaberClient(const Options& options,
              voyager::EventLoop* send_loop,
              RunLoop* event_loop);
  ~SaberClient();

  void Start();
  void Stop();

  void Create(const CreateRequest& request,
              void* context, const CreateCallback& cb);

  void Delete(const DeleteRequest& request,
              void* context, const DeleteCallback& cb);

  void Exists(const ExistsRequest& request, Watcher* watcher,
              void* context, const ExistsCallback& cb);

  void GetData(const GetDataRequest& request, Watcher* watcher,
               void* context, const GetDataCallback& cb);

  void SetData(const SetDataRequest& request,
               void* context, const SetDataCallback& cb);

  void GetACL(const GetACLRequest& request,
              void* context, const GetACLCallback& cb);

  void SetACL(const SetACLRequest& request,
              void* context, const SetACLCallback& cb);

  void GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   void* context, const ChildrenCallback& cb);

 private:
  void Connect(const voyager::SockAddr& addr);
  void Close();
  void TrySendInLoop(SaberMessage* message);
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  void OnMessage(std::unique_ptr<SaberMessage> message);

  std::atomic<bool> has_started_;

  ServerManager* server_manager_;
  voyager::EventLoop* send_loop_;
  RunLoop* event_loop_;

  std::unique_ptr<voyager::TcpClient> client_;
  std::unique_ptr<Messager> messager_;
  std::unique_ptr<ClientWatchManager> watch_manager_;

  std::queue<std::unique_ptr<Request<CreateCallback> > > create_queue_;
  std::queue<std::unique_ptr<Request<DeleteCallback> > > delete_queue_;
  std::queue<std::unique_ptr<Request<ExistsCallback> > > exists_queue_;
  std::queue<std::unique_ptr<Request<GetDataCallback> > > get_data_queue_;
  std::queue<std::unique_ptr<Request<SetDataCallback> > > set_data_queue_;
  std::queue<std::unique_ptr<Request<GetACLCallback> > > get_acl_queue_;
  std::queue<std::unique_ptr<Request<SetACLCallback> > > set_acl_queue_;
  std::queue<std::unique_ptr<Request<ChildrenCallback> > > children_queue_;

  std::deque<std::unique_ptr<SaberMessage> > outgoing_queue_;

  std::string master_ip_;
  uint16_t master_port_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
