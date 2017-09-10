// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_COMMITTER_H_
#define SABER_SERVER_COMMITTER_H_

#include <memory>
#include <string>

#include <skywalker/node.h>
#include <voyager/core/eventloop.h>

#include "saber/proto/saber.pb.h"
#include "saber/server/saber_db.h"

namespace saber {

class SaberSession;

class Committer : public std::enable_shared_from_this<Committer> {
 public:
  static uint32_t kMaxDataSize;

  Committer(uint32_t group_id, SaberSession* session, voyager::EventLoop* loop,
            SaberDB* db, skywalker::Node* node);

  void SetEventLoop(voyager::EventLoop* loop) { loop_ = loop; }

  void Commit(std::unique_ptr<SaberMessage> message);

 private:
  void HandleCommit(std::unique_ptr<SaberMessage> message);
  void Propose(std::unique_ptr<SaberMessage> message);
  void OnProposeComplete(uint64_t instance_id, const skywalker::Status& s,
                         void* context);
  void SetFailedState(SaberMessage* reply_message);

  const uint32_t group_id_;
  SaberSession* session_;
  voyager::EventLoop* loop_;
  SaberDB* db_;
  skywalker::Node* node_;

  // No copying allowed
  Committer(const Committer&);
  void operator=(const Committer&);
};

typedef std::shared_ptr<Committer> CommitterPtr;

}  // namespace saber

#endif  // SABER_SERVER_COMMITTER_H_
