// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_REQUEST_H_
#define SABER_CLIENT_REQUEST_H_

#include "saber/service/watcher.h"

namespace saber {

template<typename T>
class Request {
 public:
  std::string client_path;
  std::string server_path;
  void* context;
  Watcher* watcher;
  T callback;

  Request(std::string&& c, std::string&& s,
          void* ctx, Watcher* w, const T& cb)
      : client_path(std::move(c)),
        server_path(std::move(s)),
        context(ctx),
        watcher(w),
        callback(cb) {
  }

  Request(std::string&& c, std::string&& s,
          void* ctx, Watcher* w, T&& cb)
      : client_path(std::move(c)),
        server_path(std::move(s)),
        context(ctx),
        watcher(w),
        callback(std::move(cb)) {
  }

  Request(const Request& r) {
    client_path = r.client_path;
    server_path = r.server_path;
    context = r.context;
    watcher = r.watcher;
    callback = r.callback;
  }
  Request(Request&& r) {
    client_path = std::move(r.client_path);
    server_path = std::move(r.server_path);
    context = r.context;
    watcher = r.watcher;
    callback = std::move(r.callback);
  }
  void operator=(const Request& r) {
    client_path = r.client_path;
    server_path = r.server_path;
    context = r.context;
    watcher = r.watcher;
    callback = r.callback;
  }
  void operator=(Request&& r) {
    client_path = std::move(r.client_path);
    server_path = std::move(r.server_path);
    context = r.context;
    watcher = r.watcher;
    callback = std::move(r.callback);
  }
};

}  // namespace saber

#endif  // SABER_CLIENT_REQUEST_H_
