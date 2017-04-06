// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <saber/client/saber.h>
#include <saber/util/runloop.h>

void CreateCallback(const std::string& path, void* ctx,
                    const saber::CreateResponse& response) {
  std::cout << "create: " << response.code() << std::endl;
}

void GetDataCallback(const std::string& path, void* context,
                     const saber::GetDataResponse& response) {
  std::cout << "get data: " << response.code()
            << " data: " << response.data() << std::endl;
}

void SetDataCallback(const std::string& path, void* context,
                     const saber::SetDataResponse& response) {
  std::cout << "set data: " << response.code() << std::endl;
}

class DefaultWatcher : public saber::Watcher {
  public:
   DefaultWatcher() { }
   virtual void Process(const saber::WatchedEvent& event) {
     std::cout << "watch event: " << event.path() << std::endl;
   }
};

int main() {
  DefaultWatcher watcher;
  saber::ClientOptions options;
  options.group_size = 1;
  options.servers = "127.0.0.1:8887,127.0.0.1:8888,127.0.0.1:8889";
  saber::Saber client(options);
  client.Start();
  saber::CreateRequest r1;
  r1.set_path("/ls");
  r1.set_data("saber client test");
  client.Create(r1, nullptr, &CreateCallback);
  client.Connect();
  saber::GetDataRequest r2;
  r2.set_path("/ls");
  r2.set_watch(true);
  client.GetData(r2, &watcher, nullptr, &GetDataCallback);
  saber::SetDataRequest r3;
  r3.set_path("/ls");
  r3.set_data("saber client pass test");
  r3.set_version(-1);
  client.SetData(r3, nullptr, &SetDataCallback);
  saber::RunLoop loop;
  loop.Loop();
  return 0;
}
