// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_REQUEST_H_
#define SABER_CLIENT_REQUEST_H_

#include <string>
#include <utility>

#include "saber/service/watcher.h"

namespace saber {

template <typename T>
class Request {
 public:
  uint32_t message_id;
  std::string path;
  void* context;
  Watcher* watcher;
  T callback;

  Request(const std::string& p, void* ctx, Watcher* w, const T& cb)
      : path(p), context(ctx), watcher(w), callback(cb) {}

  Request(const std::string& p, void* ctx, Watcher* w, T&& cb)
      : path(p), context(ctx), watcher(w), callback(std::move(cb)) {}
};

}  // namespace saber

#endif  // SABER_CLIENT_REQUEST_H_
