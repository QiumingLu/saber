// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_COMMITTER_H_
#define SABER_SERVER_COMMITTER_H_

#include <memory>
#include <string>

#include <skywalker/node.h>
#include <voyager/core/eventloop.h>

#include "saber/server/saber_db.h"
#include "saber/proto/saber.pb.h"

namespace saber {

class ServerConnection;

class Committer : public std::enable_shared_from_this<Committer> {
 public:
  Committer(ServerConnection* conn, voyager::EventLoop* loop,
            SaberDB* db, skywalker::Node* node);

  void Commit(SaberMessage* message);

 private:
  void Commit(uint32_t group_id, SaberMessage* message);
  bool Propose(uint32_t group_id,
               SaberMessage* message, SaberMessage* reply_message);
  void OnProposeComplete(skywalker::MachineContext* context,
                         const skywalker::Status& s,
                         uint64_t instance_id);
  void SetFailedState(SaberMessage* reply_message);
  uint32_t Shard(const std::string& s);

  ServerConnection* conn_;
  voyager::EventLoop* loop_;
  SaberDB* db_;
  skywalker::Node* node_;
  std::unique_ptr<skywalker::MachineContext> context_;

  // No copying allowed
  Committer(const Committer&);
  void operator=(const Committer&);
};

typedef std::shared_ptr<Committer> CommitterPtr;

}  // namespace saber

#endif  // SABER_SERVER_COMMITTER_H_
