// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_SESSION_H_
#define SABER_SERVER_SABER_SESSION_H_

#include <deque>
#include <memory>
#include <utility>

#include <skywalker/node.h>

#include <voyager/core/tcp_connection.h>
#include <voyager/protobuf/protobuf_codec.h>

#include "saber/proto/saber.pb.h"
#include "saber/server/saber_db.h"
#include "saber/service/watcher.h"
#include "saber/util/mutex.h"

namespace saber {

class SaberSession : public Watcher,
                     public std::enable_shared_from_this<SaberSession> {
 public:
  static uint32_t kMaxDataSize;

  SaberSession(const std::string& root, uint32_t group_id, uint64_t session_id,
               const voyager::TcpConnectionPtr& p, SaberDB* db,
               skywalker::Node* node);
  virtual ~SaberSession();

  uint32_t group_id() const { return group_id_; }
  uint64_t session_id() const { return session_id_; }

  void set_version(uint64_t version) { version_ = version; }
  uint64_t version() const { return version_; }

  voyager::TcpConnectionPtr GetTcpConnectionPtr() const {
    return conn_wp_.lock();
  }

  void OnConnection(const voyager::TcpConnectionPtr& p);

  bool OnMessage(std::unique_ptr<SaberMessage> message);

  virtual void Process(const WatchedEvent& event);

 private:
  static void WeakCallback(std::weak_ptr<SaberSession> session_wp,
                           uint64_t instance_id, const skywalker::Status& s,
                           void* context);
  static void SetFailedState(SaberMessage* reply_message);

  void HandleMessage(std::unique_ptr<SaberMessage> message);
  void DoIt(std::unique_ptr<SaberMessage> message);
  void Done(std::unique_ptr<SaberMessage> message);
  void Propose(std::unique_ptr<SaberMessage> message);

  const std::string kRoot;

  const uint32_t group_id_;
  const uint64_t session_id_;

  uint64_t version_;
  bool closed_;
  bool last_finished_;

  voyager::ProtobufCodec<SaberMessage> codec_;
  std::weak_ptr<voyager::TcpConnection> conn_wp_;
  SaberDB* db_;
  skywalker::Node* node_;

  Mutex mutex_;
  std::deque<std::unique_ptr<SaberMessage>> pending_messages_;

  // No copying allowed
  SaberSession(const SaberSession&);
  void operator=(const SaberSession&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_SESSION_H_
