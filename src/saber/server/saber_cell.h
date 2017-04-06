// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_CELL_H_
#define SABER_SERVER_SABER_CELL_H_

#include <stdint.h>
#include <map>
#include <string>

#include "saber/util/mutex.h"

namespace saber {

class ServerMessage {
 public:
  uint16_t server_id;
  uint16_t client_port;
  uint16_t paxos_port;
  std::string ip;
};

class SaberCell {
 public:
  typedef std::pair<std::string, uint16_t> IpPort;

  static SaberCell* Instance() {
    InitOnce(&once_, &SaberCell::Init);
    return instance_;
  }

  static void ShutDown() {
    delete instance_;
    instance_ = nullptr;
  }

  void AddServer(const ServerMessage& s);
  void RemoveServer(uint16_t id);

  bool FindServerById(uint16_t id, ServerMessage* s) const;
  bool FindServerByClientIpPort(const IpPort& i, ServerMessage* s) const;
  bool FindServerByPaxosIpPort(const IpPort& i, ServerMessage* s) const;

 private:
  static void Init() {
    instance_ = new SaberCell();
  }
  static OnceType once_;
  static SaberCell* instance_;

  mutable Mutex mutex_;
  std::map<int, ServerMessage*> id_to_servers_;
  std::map<IpPort, ServerMessage*> client_to_servers_;
  std::map<IpPort, ServerMessage*> paxos_to_servers_;

  SaberCell();
  ~SaberCell();
  SaberCell(const SaberCell&);
  void operator=(const SaberCell&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_CELL_H_
