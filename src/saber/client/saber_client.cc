// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/client/saber_client.h"

#include <voyager/util/string_util.h>

#include "saber/client/server_manager_impl.h"
#include "saber/util/logging.h"

namespace saber {

SaberClient::SaberClient(voyager::EventLoop* loop,
                         const std::string& servers,
                         std::unique_ptr<ServerManager> manager)
    : loop_(loop),
      server_manager_(std::move(manager)) {
  if (!server_manager_) {
    server_manager_.reset(new ServerManagerImpl());
  }
  server_manager_->UpdateServers(servers);
}

SaberClient::~SaberClient() {
}

void SaberClient::Start() {
  Connect();
}

void SaberClient::Connect() {
  std::pair<std::string, uint16_t> s = server_manager_->GetNext();
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
  LOG_DEBUG("SaberClient::OnConnection - connect successfully!\n");
}

void SaberClient::OnFailue() {
  LOG_DEBUG("SaberClient::OnFailue - connect failed, try to next server!\n");
  Connect();
}

void SaberClient::OnClose(const voyager::TcpConnectionPtr& p) {
  LOG_DEBUG("SaberClient::OnClose - connect close!\n");
  client_.reset();
}

void SaberClient::OnMessage(
    const voyager::TcpConnectionPtr& p, voyager::Buffer* buf) {
}

}  // namespace saber
