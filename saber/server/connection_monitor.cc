#include "saber/server/connection_monitor.h"
#include "saber/server/saber_db.h"
#include "saber/server/server_connection.h"
#include "saber/util/timeops.h"
#include "saber/util/sequence_number.h"

namespace saber {

ConnectionMonitor::ConnectionMonitor(int server_id,
                                     int max_ip_connections,
                                     int max_all_connections,
                                     SaberDB* db,
                                     skywalker::Node* node)
    : server_id_(server_id),
      max_ip_connections_(max_ip_connections),
      max_all_connections_(max_all_connections),
      db_(db),
      node_(node) {
}

ConnectionMonitor::~ConnectionMonitor() {
}

void ConnectionMonitor::OnConnection(const voyager::TcpConnectionPtr& p) {
  const std::string& ip = p->PeerSockAddr().Ip();
  auto it = ip_counter_.find(ip);
  if (it != ip_counter_.end()) {
    if (++(it->second) > max_ip_connections_) {
      p->ShutDown();
      return;
    }
  } else {
    ip_counter_.insert(std::make_pair(ip, 1));
  }

  uint64_t session_id = GetNextSessionId();
  if (conns_.find(session_id) == conns_.end() &&
      voyager::EventLoop::AllConnectionSize() < max_all_connections_) {
    ServerConnection* conn = new ServerConnection(session_id, p, db_, node_);
    conns_[session_id] = std::unique_ptr<ServerConnection>(conn);
    p->SetContext(conn);
    p->SetMessageCallback(
        [conn](const voyager::TcpConnectionPtr& ptr, voyager::Buffer* buf) {
      conn->OnMessage(ptr, buf);
    });
  } else {
    p->ShutDown();
  }
}

void ConnectionMonitor::OnClose(const voyager::TcpConnectionPtr& p) {
  const std::string& ip = p->PeerSockAddr().Ip();
  auto it = ip_counter_.find(ip);
  if (it != ip_counter_.end()) {
    if (--(it->second) <= 0) {
      ip_counter_.erase(it);
    }
  }
  ServerConnection* conn =
      reinterpret_cast<ServerConnection*>(p->Context());
  if (conn != nullptr) {
    db_->RemoveWatcher(conn);
    conns_.erase(conn->session_id());
  }
}

uint64_t ConnectionMonitor::GetNextSessionId() const {
  static SequencenNumber<int> seq_num_(1 << 10);
  return (NowMillis() << 22) | (server_id_ << 10) | seq_num_.GetNext();
}

}  // namespace saber
