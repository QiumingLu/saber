// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_COMMITTER_H_
#define SABER_SERVER_COMMITTER_H_

#include <functional>

#include <skywalker/node.h>
#include <voyager/core/eventloop.h>

#include "saber/server/saber_db.h"
#include "saber/proto/saber.pb.h"

namespace saber {

class ServerConnection;

class Committer : public skywalker::StateMachine {
 public:
  typedef std::function<
      void (std::unique_ptr<SaberMessage>)> CommitCompleteCallback;

  Committer(ServerConnection* conn, SaberDB* db_, 
            voyager::EventLoop* loop, skywalker::Node* node);

  virtual ~Committer();

  void SetCommitCompleteCallback(const CommitCompleteCallback& cb) {
    cb_ = cb;
  }
  void SetCommitCompleteCallback(CommitCompleteCallback&& cb) {
    cb_ = std::move(cb);
  }

  void Commit(SaberMessage* message);

  virtual bool Execute(uint32_t group_id,
                       uint64_t instance_id,
                       const std::string& value,
                       skywalker::MachineContext* context);

 private:
  void Commit(uint32_t group_id, 
              SaberMessage* message);
  bool Propose(uint32_t group_id,
               SaberMessage* message, SaberMessage* reply_message);
  void OnProposeComplete(skywalker::MachineContext* context,
                         const skywalker::Status& s,
                         uint64_t instance_id);
  void SetFailedState(SaberMessage* reply_message);
  uint32_t Shard(const std::string& s);

  ServerConnection* conn_;
  SaberDB* db_;
  voyager::EventLoop* loop_;
  skywalker::Node* node_;
  skywalker::MachineContext* context_;
  CommitCompleteCallback cb_;

  // No copying allowed
  Committer(const Committer&);
  void operator=(const Committer&);
};

}  // namespace saber

#endif  // SABER_SERVER_COMMITTER_H_
