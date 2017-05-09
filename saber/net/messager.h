// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_NET_MESSAGER_H_
#define SABER_NET_MESSAGER_H_

#include <memory>
#include <functional>
#include <voyager/core/tcp_connection.h>
#include "saber/proto/saber.pb.h"

namespace saber {

class Messager {
 public:
  typedef std::function<
      bool (std::unique_ptr<SaberMessage>) > MessageCallback;

  Messager();
  ~Messager();

  void SetTcpConnection(const voyager::TcpConnectionPtr& p) {
    conn_wp_ = p;
  }
  voyager::TcpConnectionPtr GetTcpConnection() const {
    return conn_wp_.lock();
  }
  void SetMessageCallback(const MessageCallback& cb) { cb_ = cb; }
  void SetMessageCallback(MessageCallback&& cb) { cb_ = std::move(cb); }

  bool SendMessage(const SaberMessage& message);
  void OnMessage(const voyager::TcpConnectionPtr& p, voyager::Buffer* buf);

 private:
  static const int kHeaderSize = 4;

  std::weak_ptr<voyager::TcpConnection> conn_wp_;
  MessageCallback cb_;

  // No copying allowed
  Messager(const Messager&);
  void operator=(const Messager&);
};

}  // namespace saber

#endif  // SABER_NET_MESSAGER_H_
