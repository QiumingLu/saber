// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_SERVER_H_
#define SABER_SERVER_SABER_SERVER_H_

#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include <skywalker/node.h>

#include <voyager/core/bg_eventloop.h>
#include <voyager/core/buffer.h>
#include <voyager/core/eventloop.h>
#include <voyager/core/tcp_connection.h>
#include <voyager/core/tcp_monitor.h>
#include <voyager/core/tcp_server.h>
#include <voyager/protobuf/protobuf_codec.h>
#include <voyager/util/hash.h>

#include "saber/proto/saber.pb.h"
#include "saber/server/server_options.h"
#include "saber/util/runloop.h"
#include "saber/util/runloop_thread.h"

namespace saber {

class SaberDB;
class SaberSession;

class SaberServer {
 public:
  SaberServer(voyager::EventLoop* loop, const ServerOptions& options);
  ~SaberServer();

  bool Start();

  const skywalker::Node* GetNode() const { return node_.get(); }

 private:
  struct Context;
  struct Entry;
  typedef std::shared_ptr<Entry> EntryPtr;
  typedef std::vector<EntryPtr> Bucket;
  typedef std::vector<Bucket> BucketList;
  typedef std::unordered_map<uint64_t, std::weak_ptr<SaberSession>> SessionMap;

  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);
  bool OnMessage(const voyager::TcpConnectionPtr& p,
                 std::unique_ptr<SaberMessage> message);
  void OnError(const voyager::TcpConnectionPtr& p,
               voyager::ProtoCodecError code);
  void OnTimer();
  void UpdateBuckets(const voyager::TcpConnectionPtr& p, const EntryPtr& entry);
  bool HandleMessage(const EntryPtr& p, std::unique_ptr<SaberMessage> message);
  bool OnConnectRequest(const std::string& root, uint32_t group_id,
                        const EntryPtr& entry,
                        std::unique_ptr<SaberMessage> message);
  void OnConnectResponse(const EntryPtr& entry,
                         std::unique_ptr<SaberMessage> message);
  void OnCloseRequest(uint32_t group_id, const CloseRequest& request);
  bool CreateSession(const std::string& root, uint32_t group_id,
                     uint64_t session_id, uint64_t version,
                     const EntryPtr& entry);
  void CloseSession(const std::shared_ptr<SaberSession>& session);
  void CleanSessions(uint32_t group_id);

  void NewServers(uint32_t group_id);

  uint64_t GetNextSessionId() const;
  uint32_t Shard(const std::string& s) const;

  ServerOptions options_;

  const uint64_t server_id_;

  // 每个循环队列所含的桶的个数。
  int idle_ticks_;

  // 分桶策略
  // 每个EventLoop都有一个会话清理的循环队列。
  // Key表示每个EventLoop。
  // Value的first值表示循环队列，second值表示该队列最后一个元素，即最后一个桶。
  std::unordered_map<voyager::EventLoop*, std::pair<BucketList, int>> buckets_;

  // FIXME Use a class to manage it?
  std::vector<std::mutex> mutexes_;
  std::vector<SessionMap> sessions_;

  std::unique_ptr<SaberDB> db_;
  std::unique_ptr<skywalker::Node> node_;

  RunLoop* loop_;
  RunLoopThread thread_;

  voyager::ProtobufCodec<SaberMessage> codec_;
  voyager::TcpMonitor monitor_;
  voyager::TcpServer server_;

  // No copying allowed
  SaberServer(const SaberServer&);
  void operator=(const SaberServer&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_SERVER_H_
