// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/main/mysaber.h"

#include <vector>

#include "saber/main/to_string.h"

namespace saber {

MySaber::MySaber(RunLoop* loop, const ClientOptions& options)
    : loop_(loop), saber_(options, &watcher_) {}

MySaber::~MySaber() {}

void MySaber::Start() {
  saber_.Start();
  saber_.Connect();
  GetLine();
}

void MySaber::GetLine() {
  printf("please select:\n");
  printf("0. Create\n");
  printf("1. Delete\n");
  printf("2. Exists\n");
  printf("3. GetData\n");
  printf("4. SetData\n");
  printf("5. GetACL\n");
  printf("6. SetACL\n");
  printf("7. GetChildren\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  if (s == "quit") {
    printf("bye!\n");
    exit(0);
  }
  bool res = NewRequest(s);
  if (!res) {
    GetLine();
  }
}

bool MySaber::NewRequest(const std::string& s) {
  bool res = true;
  int type = atoi(s.c_str());
  switch (type) {
    case kCreate: {
      Create();
      break;
    }
    case kDelete: {
      Delete();
      break;
    }
    case kExists: {
      Exists();
      break;
    }
    case kGetData: {
      GetData();
      break;
    }
    case kSetData: {
      SetData();
      break;
    }
    case kGetACL: {
      GetACL();
      break;
    }
    case kSetACL: {
      SetACL();
      break;
    }
    case kGetChildren: {
      GetChildren();
      break;
    }
    default: {
      printf("Invalid request type.\n");
      res = false;
      break;
    }
  }
  return res;
}

void MySaber::Create() {
  printf("please enter: path|data|type\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 3) {
    Create();
    return;
  }
  CreateRequest request;
  request.set_path(v[0]);
  request.set_data(v[1]);
  int node_type = atoi(v[2].c_str());
  request.set_type(static_cast<NodeType>(node_type));
  saber_.Create(request, nullptr,
                [this](const std::string& path, void* context,
                       const CreateResponse& response) {
                  OnCreateReply(path, context, response);
                });
}

void MySaber::OnCreateReply(const std::string& path, void* context,
                            const CreateResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::Delete() {
  printf("please enter: path|version\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 2) {
    Delete();
    return;
  }
  DeleteRequest request;
  request.set_path(v[0]);
  request.set_version(atoi(v[1].c_str()));
  saber_.Delete(request, nullptr,
                [this](const std::string& path, void* context,
                       const DeleteResponse& response) {
                  OnDeleteReply(path, context, response);
                });
}

void MySaber::OnDeleteReply(const std::string& path, void* context,
                            const DeleteResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::Exists() {
  printf("please enter: path|watch(0|1)\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 2) {
    Exists();
    return;
  }
  ExistsRequest request;
  Watcher* watcher = nullptr;
  request.set_path(v[0]);
  if (v[1] == "0") {
    request.set_watch(false);
  } else if (v[1] == "1") {
    request.set_watch(true);
    watcher = &watcher_;
  } else {
    Exists();
    return;
  }
  saber_.Exists(request, watcher, nullptr,
                [this](const std::string& path, void* context,
                       const ExistsResponse& response) {
                  OnExistsReply(path, context, response);
                });
}

void MySaber::OnExistsReply(const std::string& path, void* context,
                            const ExistsResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::GetData() {
  printf("please enter: path|watch(0|1)\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 2) {
    GetData();
    return;
  }
  GetDataRequest request;
  Watcher* watcher = nullptr;
  request.set_path(v[0]);
  if (v[1] == "0") {
    request.set_watch(false);
  } else if (v[1] == "1") {
    request.set_watch(true);
    watcher = &watcher_;
  } else {
    GetData();
    return;
  }
  saber_.GetData(request, watcher, nullptr,
                 [this](const std::string& path, void* context,
                        const GetDataResponse& response) {
                   OnGetDataReply(path, context, response);
                 });
}

void MySaber::OnGetDataReply(const std::string& path, void* context,
                             const GetDataResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::SetData() {
  printf("please enter: path|data|version\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 3) {
    SetData();
    return;
  }
  SetDataRequest request;
  request.set_path(v[0]);
  request.set_data(v[1]);
  request.set_version(atoi(v[2].c_str()));
  saber_.SetData(request, nullptr,
                 [this](const std::string& path, void* context,
                        const SetDataResponse& response) {
                   OnSetDataReply(path, context, response);
                 });
}

void MySaber::OnSetDataReply(const std::string& path, void* context,
                             const SetDataResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::GetACL() {
  printf("please enter: path\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  GetACLRequest request;
  request.set_path(s);
  saber_.GetACL(request, nullptr,
                [this](const std::string& path, void* context,
                       const GetACLResponse& response) {
                  OnGetACLReply(path, context, response);
                });
}

void MySaber::OnGetACLReply(const std::string& path, void* context,
                            const GetACLResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::SetACL() {
  printf("please enter: path|perms:scheme:id,...|version\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 3) {
    SetACL();
    return;
  }
  SetACLRequest request;
  request.set_path(v[0]);
  std::vector<std::string> acls;
  voyager::SplitStringUsing(v[1], ",", &acls);
  for (auto& i : acls) {
    std::vector<std::string> acl;
    voyager::SplitStringUsing(i, ":", &acl);
    if (acl.size() != 3) {
      SetACL();
      return;
    }
    ACL one;
    Id* id = new Id();
    one.set_perms(atoi(v[0].c_str()));
    id->set_scheme(v[1]);
    id->set_id(v[2]);
    one.set_allocated_id(id);
    *(request.add_acl()) = one;
  }
  request.set_version(atoi(v[2].c_str()));
  saber_.SetACL(request, nullptr,
                [this](const std::string& path, void* context,
                       const SetACLResponse& response) {
                  OnSetACLReply(path, context, response);
                });
}

void MySaber::OnSetACLReply(const std::string& path, void* context,
                            const SetACLResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

void MySaber::GetChildren() {
  printf("please enter: path|watch(0|1)\n");
  printf("> ");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 2) {
    GetChildren();
    return;
  }
  GetChildrenRequest request;
  request.set_path(v[0]);
  Watcher* watcher = nullptr;
  if (v[1] == "0") {
    request.set_watch(false);
  } else if (v[1] == "1") {
    request.set_watch(true);
    watcher = &watcher_;
  } else {
    GetChildren();
    return;
  }
  saber_.GetChildren(request, watcher, nullptr,
                     [this](const std::string& path, void* context,
                            const GetChildrenResponse& response) {
                       OnGetChildrenReply(path, context, response);
                     });
}

void MySaber::OnGetChildrenReply(const std::string& path, void* context,
                                 const GetChildrenResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() { GetLine(); });
}

}  // namespace saber
