// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <iostream>
#include <voyager/util/string_util.h>
#include <saber/util/logging.h>
#include <saber/util/runloop.h>
#include <saber/client/saber.h>

namespace saber {

std::string ToString(const CreateResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const DeleteResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const ExistsResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const GetDataResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const SetDataResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const GetACLResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const SetACLResponse& response) {
  std::string s;
  return s;
}

std::string ToString(const GetChildrenResponse& response) {
  std::string s;
  return s;
}

class MySaber {
 public:
  MySaber(RunLoop* loop, const ClientOptions& options);
  ~MySaber();

  void Start();

 private:
  void GetLine();
  bool NewRequest(const std::string& s);
  void Create();
  void OnCreateReply(const std::string& path, void* context,
                     const CreateResponse& response);
  void Delete();
  void OnDeleteReply(const std::string& path, void* context,
                     const DeleteResponse& response);
  void Exists();
  void OnExistsReply(const std::string& path, void* context,
                     const ExistsResponse& response);
  void GetData();
  void OnGetDataReply(const std::string& path, void* context,
                      const GetDataResponse& response);
  void SetData();
  void OnSetDataReply(const std::string& path, void* context,
                      const SetDataResponse& response);
  void GetACL();
  void OnGetACLReply(const std::string& path, void* context,
                     const GetACLResponse& response);
  void SetACL();
  void OnSetACLReply(const std::string& path, void* context,
                     const SetACLResponse& response);
  void GetChildren();
  void OnGetChildrenReply(const std::string& path, void* context,
                           const GetChildrenResponse& response);

  RunLoop* loop_;
  Saber saber_;

  // No copying allowed
  MySaber(const MySaber&);
  void operator=(const MySaber&);
};

MySaber::MySaber(RunLoop* loop, const ClientOptions& options)
    : loop_(loop),
      saber_(options) {

}

MySaber::~MySaber() {

}

void MySaber::Start() {
  saber_.Start();
  saber_.Connect();
}

void MySaber::GetLine() {
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
  if (s == "Create") {
    Create();
  } else if (s == "Delete") {
    Delete();
  } else if (s == "Exists") {
    Exists();
  } else if (s == "GetData") {
    GetData();
  } else if (s == "SetData") {
    SetData();
  } else if (s == "GetACL") {
    GetACL();
  } else if (s == "SetACL") {
    SetACL();
  } else if (s == "GetChildren") {
    GetChildren();
  } else {
    res = false;
  }
  return res;
}

void MySaber::Create() {
  printf("please enter: path|data|type\n");
  printf("> \n");
  std::string s;
  std::getline(std::cin, s);
  std::vector<std::string> v;
  voyager::SplitStringUsing(s, "|", &v);
  if (v.size() != 3) {
    Create();
  }
  CreateRequest request;
  request.set_path(v[0]);
  request.set_data(v[1]);
//  request.set_type(atoi(v[2].c_str()));
  saber_.Create(
      request, nullptr,
      [this](const std::string& path, void* context,
             const CreateResponse& response) {
    OnCreateReply(path, context, response);
  });
}

void MySaber::OnCreateReply(const std::string& path, void* context,
                            const CreateResponse& response) {
  printf("%s\n", ToString(response).c_str());
  loop_->RunInLoop([this]() {
    GetLine();
  });
}

void MySaber::Delete() {
}

void MySaber::OnDeleteReply(const std::string& path, void* context,
                            const DeleteResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::Exists() {
}

void MySaber::OnExistsReply(const std::string& path, void* context,
                            const ExistsResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::GetData() {
}

void MySaber::OnGetDataReply(const std::string& path, void* context,
                             const GetDataResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::SetData() {
}

void MySaber::OnSetDataReply(const std::string& path, void* context,
                             const SetDataResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::GetACL() {
}

void MySaber::OnGetACLReply(const std::string& path, void* context,
                            const GetACLResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::SetACL() {
}

void MySaber::OnSetACLReply(const std::string& path, void* context,
                            const SetACLResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

void MySaber::GetChildren() {
}

void MySaber::OnGetChildrenReply(const std::string& path, void* context,
                                 const GetChildrenResponse& response) {
  printf("%s\n", ToString(response).c_str());
  GetLine();
}

class DefaultWatcher : public saber::Watcher {
  public:
   DefaultWatcher() { }

   virtual void Process(const saber::WatchedEvent& event) {
     std::cout << "watch event: " << event.path() << std::endl;
   }
};

}  // namespace saber

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s server_ip:server_port,...\n", argv[0]);
    return -1;
  }

  saber::ClientOptions options;
  options.group_size = 3;
  options.servers = argv[1];
  saber::RunLoop loop;
  saber::MySaber mysaber(&loop, options);
  mysaber.Start();
  loop.Loop();
  return 0;
}
