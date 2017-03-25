// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include <memory>
#include <voyager/core/eventloop.h>
#include <voyager/core/sockaddr.h>
#include <voyager/core/tcp_client.h>

#include "saber/client/callbacks.h"
#include "saber/client/server_manager.h"
#include "saber/service/create_mode.h"
#include "saber/service/watcher.h"
#include "saber/net/messager.h"
#include "saber/util/blockingqueue.h"

namespace saber {

class SaberClient : public std::enable_shared_from_this<SaberClient> {
 public:
  SaberClient(
      voyager::EventLoop* loop,
      const std::string& server,
      std::unique_ptr<ServerManager> p = std::unique_ptr<ServerManager>());

  ~SaberClient();

  void Start();
  void Stop();

  bool Create(const CreateRequest& request, NodeType type,
              void* context, const StringCallback& cb);

  bool Delete(const DeleteRequest& request,
              void* context, const VoidCallback& cb);

  bool Exists(const ExistsRequest& request, Watcher* watcher,
              void* context, const StatCallback& cb);

  bool GetData(const GetDataRequest& request, Watcher* watcher,
               void* context, const DataCallback& cb);

  bool SetData(const SetDataRequest& request,
               void* context, const StatCallback& cb);

  bool GetACL(const GetACLRequest& request,
              void* context, const ACLCallback& cb);

  bool SetACL(const SetACLRequest& request,
              void* context, const StatCallback& cb);

  bool GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                   void* context, const ChildrenCallback& cb);

 private:
  std::string HeaderMessage(NodeType type = PERSISTENT);

  void Connect();
  void Close();
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnFailue();
  void OnClose(const voyager::TcpConnectionPtr& p);
  void OnMessage(std::unique_ptr<SaberMessage> message);

  voyager::EventLoop* loop_;
  std::unique_ptr<voyager::TcpClient> client_;
  std::unique_ptr<ServerManager> server_manager_;
  std::unique_ptr<Messager> messager_;

  std::atomic<bool> has_started_;

  BlockingQueue<std::pair<StringCallback, void*> > create_cb_;
  BlockingQueue<std::pair<VoidCallback, void*> > delete_cb_;
  BlockingQueue<std::pair<StatCallback, void*> > exists_cb_;
  BlockingQueue<std::pair<DataCallback, void*> > get_data_cb_;
  BlockingQueue<std::pair<StatCallback, void*> > set_data_cb_;
  BlockingQueue<std::pair<ACLCallback, void*> > get_acl_cb_;
  BlockingQueue<std::pair<StatCallback, void*> > set_acl_cb_;
  BlockingQueue<std::pair<ChildrenCallback, void*> > children_cb_;

  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

typedef std::shared_ptr<SaberClient> SaberClientPtr;

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
