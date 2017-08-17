// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber.h"
#include "saber/client/saber_client.h"
#include "saber/client/server_manager_impl.h"
#include "saber/util/logging.h"
#include "saber/util/murmurhash3.h"

namespace saber {

Saber::Saber(const ClientOptions& options, Watcher* watcher)
    : options_(options),
      watcher_(watcher),
      server_manager_(options.server_manager),
      loop_(nullptr) {}

Saber::~Saber() {}

bool Saber::Start() {
  if (!server_manager_) {
    server_manager_.reset(new ServerManagerImpl());
    options_.server_manager = server_manager_.get();
  }
  server_manager_->UpdateServers(options_.servers);

  loop_ = thread_.Loop();
  client_.reset(new SaberClient(loop_, options_, watcher_));
  return true;
}

void Saber::Connect() { client_->Start(); }

void Saber::Close() { client_->Stop(); }

void Saber::Create(const CreateRequest& request, void* context,
                   const CreateCallback& cb) {
  client_->Create(request, context, cb);
}

void Saber::Delete(const DeleteRequest& request, void* context,
                   const DeleteCallback& cb) {
  client_->Delete(request, context, cb);
}

void Saber::Exists(const ExistsRequest& request, Watcher* watcher,
                   void* context, const ExistsCallback& cb) {
  client_->Exists(request, watcher, context, cb);
}

void Saber::GetData(const GetDataRequest& request, Watcher* watcher,
                    void* context, const GetDataCallback& cb) {
  client_->GetData(request, watcher, context, cb);
}

void Saber::SetData(const SetDataRequest& request, void* context,
                    const SetDataCallback& cb) {
  client_->SetData(request, context, cb);
}

void Saber::GetACL(const GetACLRequest& request, void* context,
                   const GetACLCallback& cb) {
  client_->GetACL(request, context, cb);
}

void Saber::SetACL(const SetACLRequest& request, void* context,
                   const SetACLCallback& cb) {
  client_->SetACL(request, context, cb);
}

void Saber::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                        void* context, const GetChildrenCallback& cb) {
  client_->GetChildren(request, watcher, context, cb);
}

}  // namespace saber
