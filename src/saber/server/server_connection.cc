#include "saber/server/server_connection.h"
#include "saber/util/logging.h"

namespace saber {

ServerConnection::ServerConnection(std::unique_ptr<Messager> p,
                                   DataBase* db)
    : messager_(std::move(p)),
      loop_(messager_->GetTcpConnection()->OwnerEventLoop()),
      db_(db) {
  messager_->SetMessageCallback(
      [this](std::unique_ptr<SaberMessage> message) {
    OnMessage(std::move(message));
  });
}

ServerConnection::~ServerConnection() {
}

void ServerConnection::Process(const WatchedEvent& event) {
  SaberMessage message;
  message.set_type(MT_NOTIFICATION);
  message.set_data(event.SerializeAsString());
  messager_->SendMessage(message);
}

void ServerConnection::OnMessage(std::unique_ptr<SaberMessage> message) {
  switch (message->type()) {
    case MT_NOTIFICATION: {
      WatchedEvent event;
      event.ParseFromString(message->data());
      break;
    }
    case MT_CREATE: {
      CreateRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_DELETE: {
      DeleteRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_EXISTS: {
      ExistsRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_GETDATA: {
      GetDataRequest request;
      GetDataResponse response;
      request.ParseFromString(message->data());
      Watcher* watcher = request.watch() ? this : nullptr;
      db_->GetData(request.path(), watcher, &response);
      break;
    }
    case MT_SETDATA: {
      SetDataRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_GETACL: {
      GetACLRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_SETACL: {
      SetACLRequest request;
      request.ParseFromString(message->data());
      break;
    }
    case MT_GETCHILDREN: {
      GetChildrenRequest request;
      request.ParseFromString(message->data());
      break;
    }
    default: {
      LOG_ERROR("Invalid message type.\n");
      break;
    }
  }
}

}  // namespace saber
