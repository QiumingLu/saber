// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
#define SABER_CLIENT_CLIENT_WATCH_MANAGER_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "saber/service/watcher.h"

namespace saber {

class ClientWatchManager {
 public:
  explicit ClientWatchManager(Watcher* watcher = nullptr);
  ~ClientWatchManager();

  void AddDataWatcher(const std::string& path, Watcher* watcher);
  void AddChildWatcher(const std::string& path, Watcher* watcher);

  void TriggerWatcher(const WatchedEvent& event);

 private:
  Watcher* watcher_;
  std::unordered_map<std::string, std::unordered_set<Watcher*>> data_watches_;
  std::unordered_map<std::string, std::unordered_set<Watcher*>> child_watches_;

  // No copying allowed
  ClientWatchManager(const ClientWatchManager&);
  void operator=(const ClientWatchManager&);
};

}  // namespace saber

#endif  // SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
