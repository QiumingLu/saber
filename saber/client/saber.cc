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

bool Saber::Create(const CreateRequest& request, void* context,
                   const CreateCallback& cb) {
  return client_->Create(request, context, cb);
}

bool Saber::Delete(const DeleteRequest& request, void* context,
                   const DeleteCallback& cb) {
  return client_->Delete(request, context, cb);
}

bool Saber::Exists(const ExistsRequest& request, Watcher* watcher,
                   void* context, const ExistsCallback& cb) {
  return client_->Exists(request, watcher, context, cb);
}

bool Saber::GetData(const GetDataRequest& request, Watcher* watcher,
                    void* context, const GetDataCallback& cb) {
  return client_->GetData(request, watcher, context, cb);
}

bool Saber::SetData(const SetDataRequest& request, void* context,
                    const SetDataCallback& cb) {
  return client_->SetData(request, context, cb);
}

bool Saber::GetACL(const GetACLRequest& request, void* context,
                   const GetACLCallback& cb) {
  return client_->GetACL(request, context, cb);
}

bool Saber::SetACL(const SetACLRequest& request, void* context,
                   const SetACLCallback& cb) {
  return client_->SetACL(request, context, cb);
}

bool Saber::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                        void* context, const GetChildrenCallback& cb) {
  return client_->GetChildren(request, watcher, context, cb);
}

}  // namespace saber
