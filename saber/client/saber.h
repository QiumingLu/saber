// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_H_
#define SABER_CLIENT_SABER_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <voyager/core/bg_eventloop.h>
#include <voyager/core/eventloop.h>

#include "saber/client/callbacks.h"
#include "saber/client/client_options.h"
#include "saber/proto/saber.pb.h"
#include "saber/service/watcher.h"
#include "saber/util/runloop.h"
#include "saber/util/runloop_thread.h"

namespace saber {

class SaberClient;
class ServerManager;

class Saber {
 public:
  explicit Saber(const ClientOptions& options);
  ~Saber();

  bool Start();

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
  std::string GetRoot(const std::string& path);
  uint32_t Shard(const std::string& root);

  ClientOptions options_;
  std::atomic<bool> has_connected_;
  std::unique_ptr<ServerManager> server_manager_;
  std::vector<std::unique_ptr<SaberClient> > clients_;

  voyager::EventLoop* send_loop_;
  RunLoop* event_loop_;

  voyager::BGEventLoop send_thread_;
  RunLoopThread event_thread_;

  // No copying allowed
  Saber(const Saber&);
  void operator=(const Saber&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_H_
