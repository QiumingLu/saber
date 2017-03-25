#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(std::unique_ptr<Messager> p,
                                   DataBase* db)
    : messager_(std::move(p)),
      db_(db) {
  messager_->SetMessageCallback([this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_msg(event.SerializeAsString());
  messager_->SendMessage(message);
}

void ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  switch (message->type()) {
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
