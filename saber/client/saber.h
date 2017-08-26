// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_H_
#define SABER_CLIENT_SABER_H_

#include <memory>

#include <voyager/core/eventloop.h>

#include "saber/client/callbacks.h"
#include "saber/client/client_options.h"
#include "saber/proto/saber.pb.h"

namespace saber {

class SaberClient;

class Saber {
 public:
  Saber(voyager::EventLoop* loop, const ClientOptions& options);
  ~Saber();

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
  std::unique_ptr<SaberClient> client_;

  // No copying allowed
  Saber(const Saber&);
  void operator=(const Saber&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_H_
