// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber.h"
#include "saber/client/saber_client.h"
#include "saber/util/logging.h"

namespace saber {

Saber::Saber(voyager::EventLoop* loop, const ClientOptions& options)
    : connect_(false), client_(std::make_shared<SaberClient>(loop, options)) {}

Saber::~Saber() {
  if (connect_) {
    client_->Close();
  }
}

void Saber::Connect() {
  bool expected = false;
  if (connect_.compare_exchange_strong(expected, true)) {
    client_->Connect();
  } else {
    LOG_WARN("Saber client has connected, don't call it again!");
  }
}

void Saber::Close() {
  bool expected = true;
  if (connect_.compare_exchange_strong(expected, false)) {
    client_->Close();
  } else {
    LOG_WARN("Saber client has closed, don't call it again!");
  }
}

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

bool Saber::GetChildren(const GetChildrenRequest& request, Watcher* watcher,
                        void* context, const GetChildrenCallback& cb) {
  return client_->GetChildren(request, watcher, context, cb);
}

}  // namespace saber
