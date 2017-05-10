// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_REQUEST_H_
#define SABER_CLIENT_REQUEST_H_

#include <utility>
#include <string>

#include "saber/service/watcher.h"

namespace saber {

template<typename T>
class Request {
 public:
  std::string path;
  void* context;
  Watcher* watcher;
  T callback;

  Request(const std::string& p, void* ctx, Watcher* w, const T& cb)
      : path(p),
        context(ctx),
        watcher(w),
        callback(cb) {
  }

  Request(const std::string& p, void* ctx, Watcher* w, T&& cb)
      : path(p),
        context(ctx),
        watcher(w),
        callback(std::move(cb)) {
  }

  Request(const Request& r) {
    path = r.path;
    context = r.context;
    watcher = r.watcher;
    callback = r.callback;
  }

  Request(Request&& r) {
    path = std::move(r.path);
    context = r.context;
    watcher = r.watcher;
    callback = std::move(r.callback);
  }

  void operator=(const Request& r) {
    path = r.path;
    context = r.context;
    watcher = r.watcher;
    callback = r.callback;
  }

  void operator=(Request&& r) {
    path = std::move(r.path);
    context = r.context;
    watcher = r.watcher;
    callback = std::move(r.callback);
  }
};

}  // namespace saber

#endif  // SABER_CLIENT_REQUEST_H_
