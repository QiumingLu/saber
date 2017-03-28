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
  std::string client_path_;
  std::string server_path_;
  std::string path_;
  void* context_;
  Watcher* watcher_;
  T cb_;

  Request(const std::string& p, void* context, Watcher* watcher, const T& cb)
      : path_(p), context_(context), watcher_(watcher), cb_(cb) {
  }

  Request(const std::string& p, void* context, Watcher* watcher, T&& cb)
      : path_(p), context_(context), watcher_(watcher), cb_(std::move(cb)) {
  }

  Request(const Request& r) {
    path_ = r.path_;
    context_ = r.context_;
    watcher_ = r.watcher_;
    cb_ = r.cb_;
  }
  Request(Request&& r) {
    path_ = std::move(r.path_);
    context_ = r.context_;
    watcher_ = r.watcher_;
    cb_ = std::move(r.cb_);
  }
  void operator=(const Request& r) {
    path_ = r.path_;
    context_ = r.context_;
    watcher_ = r.watcher_;
    cb_ = r.cb_;
  }
  void operator=(Request&& r) {
    path_ = std::move(r.path_);
    context_ = r.context_;
    watcher_ = r.watcher_;
    cb_ = std::move(r.cb_);
  }
};

}  // namespace saber

#endif  // SABER_CLIENT_REQUEST_H_
