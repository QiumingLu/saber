// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_SERVER_H_
#define SABER_SERVER_SABER_SERVER_H_

#include <atomic>
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>

#include <voyager/core/buffer.h>
#include <voyager/core/eventloop.h>
#include <voyager/core/sockaddr.h>
#include <voyager/core/tcp_connection.h>
#include <voyager/core/tcp_server.h>

#include <skywalker/node.h>

#include "saber/server/server_options.h"
#include "saber/util/concurrent_map.h"
#include "saber/util/sequence_number.h"

namespace saber {

class SaberDB;
class ServerConnection;
class ConnectionMonitor;

class SaberServer {
 public:
  SaberServer(voyager::EventLoop* loop, const ServerOptions& options);
  ~SaberServer();

  bool Start();

 private:
  void OnConnection(const voyager::TcpConnectionPtr& p);
  void OnClose(const voyager::TcpConnectionPtr& p);
  void OnMessage(const voyager::TcpConnectionPtr& p, voyager::Buffer* buf);
  void OnTimer();
  uint64_t GetNextSessionId() const;

  struct Context;

  ServerOptions options_;

  const int server_id_;
  voyager::SockAddr addr_;

  std::unique_ptr<SaberDB> db_;
  std::unique_ptr<skywalker::Node> node_;
  std::unique_ptr<ConnectionMonitor> monitor_;

  typedef std::shared_ptr<ServerConnection> ServerConnectionPtr;
  typedef std::unordered_set<ServerConnectionPtr> Bucket;
  typedef std::vector<Bucket> BucketList;

  // 每个循环队列所含的桶的个数。
  int idle_;

  // 分桶策略
  // 每个EventLoop都有一个会话清理的循环队列。
  // Key表示每个EventLoop。
  // Value的first值表示循环队列，second值表示该队列最后一个元素，即最后一个桶。
  std::map<voyager::EventLoop*, std::pair<BucketList, int> > buckets_;

  voyager::TcpServer server_;

  // No copying allowed
  SaberServer(const SaberServer&);
  void operator=(const SaberServer&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_SERVER_H_