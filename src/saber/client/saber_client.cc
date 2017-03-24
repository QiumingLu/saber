// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <voyager/util/string_util.h>

#include "saber/client/server_manager.h"

namespace saber {

SaberClient::SaberClient(voyager::EventLoop* loop,
                         const std::string& servers)
    : loop_(loop) {
  ServerManager::Instance()->UpdateServers(servers);
}

SaberClient::~SaberClient() {
}

void SaberClient::Start() {
  Connect();
}

void SaberClient::Connect() {
  std::pair<std::string, uint16_t> s = ServerManager::Instance()->GetNext();
  voyager::SockAddr addr(s.first, s.second);
  client_.reset(new voyager::TcpClient(loop_, addr, "SaberClient"));
  client_->SetConnectionCallback(
      [this](const voyager::TcpConnectionPtr& p) {
    OnConnection(p);
  });
  client_->SetConnectFailureCallback([this]() {
    OnFailue();
  });
  client_->SetCloseCallback(
      [this](const voyager::TcpConnectionPtr& p) {
    OnClose(p);
  });
  client_->SetMessageCallback(
      [this](const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
    OnMessage(p, buf);
  });
  client_->Connect(false);
}

void SaberClient::OnConnection(const voyager::TcpConnectionPtr& p) {
}

void SaberClient::OnFailue() {
  Connect();
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
}

void SaberClient::OnMessage(
    const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
}

}  // namespace saber
