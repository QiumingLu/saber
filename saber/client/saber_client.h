// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include <atomic>
#include <deque>
#include <memory>
#include <queue>
#include <string>

#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_client.h>
#include <voyager/protobuf/protobuf_codec.h>

#include "saber/client/callbacks.h"
#include "saber/client/client_options.h"
#include "saber/client/client_watch_manager.h"
#include "saber/client/saber_request.h"
#include "saber/client/server_manager.h"
#include "saber/client/server_manager_impl.h"
#include "saber/service/watcher.h"

namespace saber {

class SaberClient {
 public:
  SaberClient(voyager::EventLoop* loop, const ClientOptions& options);
  ~SaberClient();

  void Connect();
  void Close();

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
  void TrySendInLoop(SaberMessage* message);
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  bool OnMessage(const voyager::TcpConnectionPtr& p,
                 std::unique_ptr<SaberMessage> message);
  void OnError(const voyager::TcpConnectionPtr& p,
               voyager::ProtoCodecError code);
  void OnTimer();
  void OnNotification(SaberMessage* message);
  void OnConnect(SaberMessage* message);
  bool OnCreate(SaberMessage* message);
  bool OnDelete(SaberMessage* message);
  bool OnExists(SaberMessage* message);
  bool OnGetData(SaberMessage* message);
  bool OnSetData(SaberMessage* message);
  bool OnGetACL(SaberMessage* message);
  bool OnSetACL(SaberMessage* message);
  bool OnGetChildren(SaberMessage* message);
  void TriggerWatchers(const WatchedEvent& event);
  std::string GetRoot(const std::string& path) const;

  const std::string kRoot;

  std::atomic<bool> has_started_;
  uint32_t message_id_;
  uint64_t session_id_;

  voyager::EventLoop* loop_;
  ServerManager* server_manager_;
  ServerManagerImpl* server_manager_impl_;

  ClientWatchManager watch_manager_;
  voyager::ProtobufCodec<SaberMessage> codec_;
  std::unique_ptr<voyager::TcpClient> client_;

  std::queue<std::unique_ptr<CreateRequestT> > create_queue_;
  std::queue<std::unique_ptr<DeleteRequestT> > delete_queue_;
  std::queue<std::unique_ptr<ExistsRequestT> > exists_queue_;
  std::queue<std::unique_ptr<GetDataRequestT> > get_data_queue_;
  std::queue<std::unique_ptr<SetDataRequestT> > set_data_queue_;
  std::queue<std::unique_ptr<GetACLRequestT> > get_acl_queue_;
  std::queue<std::unique_ptr<SetACLRequestT> > set_acl_queue_;
  std::queue<std::unique_ptr<GetChildrenRequestT> > children_queue_;

  std::deque<std::unique_ptr<SaberMessage> > outgoing_queue_;

  voyager::TimerId timer_;
  Master master_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
