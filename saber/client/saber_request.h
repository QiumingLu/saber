// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_REQUEST_H_
#define SABER_CLIENT_SABER_REQUEST_H_

#include <string>

#include "saber/service/watcher.h"

namespace saber {

template <typename Callback>
class SaberRequest {
 public:
  uint32_t message_id;
  std::string path;
  Watcher* watcher;
  void* context;
  Callback callback;

  SaberRequest(const std::string& p, Watcher* w, void* ctx, const Callback& cb)
      : path(p), watcher(w), context(ctx), callback(cb) {}
};

typedef SaberRequest<CreateCallback> CreateRequestT;
typedef SaberRequest<DeleteCallback> DeleteRequestT;
typedef SaberRequest<ExistsCallback> ExistsRequestT;
typedef SaberRequest<GetDataCallback> GetDataRequestT;
typedef SaberRequest<SetDataCallback> SetDataRequestT;
typedef SaberRequest<GetACLCallback> GetACLRequestT;
typedef SaberRequest<SetACLCallback> SetACLRequestT;
typedef SaberRequest<GetChildrenCallback> GetChildrenRequestT;

}  // namespace saber

#endif  // SABER_CLIENT_SABER_REQUEST_H_
