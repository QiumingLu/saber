#include "saber/server/server_connection.h"

namespace saber {

ServerConnection::ServerConnection(const voyager::TcpConnectionPtr& p)
    : conn_ptr_(p) {
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  std::string s;
  conn_ptr_->SendMessage(s);
}

}  // namespace saber
