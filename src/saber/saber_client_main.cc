// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <saber/client/saber_client.h>

void CreateCallback(int rc, const std::string& path, void* ctx,
                    const saber::CreateResponse& response) {

}

void GetDataCallback(int rc, const std::string& path, void* context,
                     const saber::GetDataResponse& response) {
}

void SetDataCallback(int rc, const std::string& path, void* context,
                     const saber::SetDataResponse& response) {
}

class DefaultWatcher : public saber::Watcher {
  public:
   DefaultWatcher() { }
   virtual void Process(const saber::WatchedEvent& event) {
   }
};

int main() {
  DefaultWatcher watcher;
  saber::SaberClient client("127.0.0.1:8888,127.0.0.1:8889");
  saber::CreateRequest r1;
  client.Create(r1, nullptr, &CreateCallback);
  client.Start();
  saber::GetDataRequest r2;
  client.GetData(r2, &watcher, nullptr, &GetDataCallback);
  saber::SetDataRequest r3;
  client.SetData(r3, nullptr, &SetDataCallback);
  while (true) {

  }
  return 0;
}
