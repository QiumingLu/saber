// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber.h"
#include "saber/client/saber_client.h"

namespace saber {

Saber::Saber(voyager::EventLoop* loop, const ClientOptions& options)
    : client_(new SaberClient(loop, options)) {}

Saber::~Saber() {}

void Saber::Connect() { client_->Connect(); }

void Saber::Close() { client_->Close(); }

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
