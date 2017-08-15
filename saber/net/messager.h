// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_NET_MESSAGER_H_
#define SABER_NET_MESSAGER_H_

#include <functional>
#include <memory>
#include <utility>

#include <voyager/core/tcp_connection.h>

#include "saber/proto/saber.pb.h"

namespace saber {

class Messager {
 public:
  typedef std::function<bool(std::unique_ptr<SaberMessage>)> MessageCallback;

  static bool SendMessage(const voyager::TcpConnectionPtr& p,
                          const SaberMessage& message);
  static void OnMessage(const voyager::TcpConnectionPtr& p,
                        voyager::Buffer* buf, const MessageCallback& cb);

 private:
  static const int kHeaderSize = 4;
};

}  // namespace saber

#endif  // SABER_NET_MESSAGER_H_
