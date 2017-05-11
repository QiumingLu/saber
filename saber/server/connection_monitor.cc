#include "saber/server/connection_monitor.h"
#include "saber/util/logging.h"
#include "saber/util/mutexlock.h"

namespace saber {

ConnectionMonitor::ConnectionMonitor(int max_all_connections,
                                     int max_ip_connections)
    : max_all_connections_(max_all_connections),
      max_ip_connections_(max_ip_connections) {
}

ConnectionMonitor::~ConnectionMonitor() {
}

bool ConnectionMonitor::OnConnection(const voyager::TcpConnectionPtr& p) {
  bool result = true;
  const std::string& ip = p->PeerSockAddr().Ip();
  mutex_.Lock();
  auto it = ip_counter_.find(ip);
  if (it != ip_counter_.end()) {
    if (++(it->second) > max_ip_connections_) {
      LOG_WARN("the connection size of ip=%s is %d, more than %d, "
               "so force close it.",
               ip.c_str(), it->second, max_ip_connections_);
      result = false;
    }
  } else {
    ip_counter_.insert(std::make_pair(ip, 1));
  }
  mutex_.UnLock();

  // FIXME Maybe can disable listening in voyager?
  if (voyager::EventLoop::AllConnectionSize() > max_all_connections_) {
    LOG_WARN("all connection size is %d, more than %d, so force close it.",
             voyager::EventLoop::AllConnectionSize(), max_all_connections_);
    result = false;
  }
  if (!result) {
    p->ForceClose();
  }
  return result;
}

void ConnectionMonitor::OnClose(const voyager::TcpConnectionPtr& p) {
  const std::string& ip = p->PeerSockAddr().Ip();
  MutexLock lock(&mutex_);
  auto it = ip_counter_.find(ip);
  if (it != ip_counter_.end()) {
    if (--(it->second) <= 0) {
      ip_counter_.erase(it);
    }
  }
}

}  // namespace saber
