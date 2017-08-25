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
#include <voyager/protobuf/protobuf_codec.h>

#include "saber/client/callbacks.h"
#include "saber/client/client_options.h"
#include "saber/client/request.h"
#include "saber/client/server_manager.h"
#include "saber/service/watcher.h"

namespace saber {

class Messager;
class ClientWatchManager;

class SaberClient {
 public:
  SaberClient(voyager::EventLoop* loop, const ClientOptions& options,
              Watcher* watcher);
  ~SaberClient();

  void Start();
  void Stop();

  void Create(const CreateRequest& request, void* context,
              const CreateCallback& cb);

  void Delete(const DeleteRequest& request, void* context,
              const DeleteCallback& cb);

  void Exists(const ExistsRequest& request, Watcher* watcher, void* context,
              const ExistsCallback& cb);

  void GetData(const GetDataRequest& request, Watcher* watcher, void* context,
               const GetDataCallback& cb);

  void SetData(const SetDataRequest& request, void* context,
               const SetDataCallback& cb);

  void GetACL(const GetACLRequest& request, void* context,
              const GetACLCallback& cb);

  void SetACL(const SetACLRequest& request, void* context,
              const SetACLCallback& cb);

  void GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   void* context, const GetChildrenCallback& cb);

 private:
  void Connect(const voyager::SockAddr& addr);
  void Close();
  void TrySendInLoop(SaberMessage* message);
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  bool OnMessage(const voyager::TcpConnectionPtr& p,
                 std::unique_ptr<SaberMessage> message);
  void OnError(const voyager::TcpConnectionPtr& p,
               voyager::ProtoCodecError code);
  void TriggerWatchers(WatchedEvent* event);

  std::atomic<bool> has_started_;
  const std::string root_;

  ServerManager* server_manager_;
  voyager::EventLoop* loop_;

  voyager::ProtobufCodec<SaberMessage> codec_;

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

  int message_id_;
  std::deque<std::unique_ptr<SaberMessage> > outgoing_queue_;

  Master master_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
