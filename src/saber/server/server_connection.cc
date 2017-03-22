#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(const voyager::TcpConnectionPtr& p,
                                   DataBase* db)
    : conn_ptr_(p),
      loop_(conn_ptr_->OwnerEventLoop()),
      db_(db) {
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  loop_->RunInLoop([this, event]() {
    std::string s;
    char buf[kHeaderSize];
    memset(buf, 0, kHeaderSize);
    s.append(buf, kHeaderSize);
    bool res = event.AppendToString(&s);
    if (res) {
      int size = static_cast<int>(s.size());
      memcpy(buf, &size, kHeaderSize);
      s.replace(s.begin(), s.begin() + kHeaderSize, buf, kHeaderSize);
      conn_ptr_->SendMessage(s);
    } else {
      LOG_ERROR("WatchedEvent serialize to string failed.\n");
    }
  });
}

void ServerConnection::OnMessage(voyager::Buffer* buf) {
  while (true) {
    if (buf->ReadableSize() >= kHeaderSize) {
      int size;
      memcpy(&size, buf->Peek(), kHeaderSize);
      if (buf->ReadableSize() >= static_cast<size_t>(size)) {
        SaberMessage msg;
        msg.ParseFromArray(buf->Peek() + kHeaderSize, size - kHeaderSize);
        HandleMessage(msg);
        buf->Retrieve(size);
      } else {
        break;
      }
    } else {
      break;
    }
  }
}

void ServerConnection::HandleMessage(const SaberMessage& msg) {
  switch (msg.type()) {
    case MT_NOTIFICATION:
      break;
    case MT_CREATE:
      break;
    case MT_DELETE:
      break;
    case MT_EXISTS:
      break;
    case MT_GETDATA:
      break;
    case MT_SETDATA:
      break;
    case MT_GETACL:
      break;
    case MT_SETACL:
      break;
    case MT_GETCHILDREN:
      break;
    default:
      LOG_ERROR("Invalid message type.\n");
      break;
  }
}

}  // namespace saber
