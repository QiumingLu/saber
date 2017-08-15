// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/net/messager.h"

#include <string>

#include <voyager/core/buffer.h>

#include "saber/util/logging.h"

namespace saber {

bool Messager::SendMessage(const voyager::TcpConnectionPtr& p,
                           const SaberMessage& message) {
  bool res = false;
  if (p) {
    std::string s;
    char buf[kHeaderSize];
    memset(buf, 0, kHeaderSize);
    s.append(buf, kHeaderSize);
    res = message.AppendToString(&s);
    if (res) {
      int size = static_cast<int>(s.size());
      memcpy(buf, &size, kHeaderSize);
      s.replace(s.begin(), s.begin() + kHeaderSize, buf, kHeaderSize);
      p->SendMessage(std::move(s));
    } else {
      LOG_ERROR("SaberMessage serialize to string failed.");
    }
  }
  return res;
}

void Messager::OnMessage(const voyager::TcpConnectionPtr& p,
                         voyager::Buffer* buf, const MessageCallback& cb) {
  bool res = true;
  while (res) {
    if (buf->ReadableSize() >= kHeaderSize) {
      int size;
      memcpy(&size, buf->Peek(), kHeaderSize);
      if (buf->ReadableSize() >= static_cast<size_t>(size)) {
        SaberMessage* message = new SaberMessage();
        message->ParseFromArray(buf->Peek() + kHeaderSize, size - kHeaderSize);
        res = cb(std::unique_ptr<SaberMessage>(message));
        buf->Retrieve(size);
        continue;
      }
    }
    break;
  }
}

}  // namespace saber
