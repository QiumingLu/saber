// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
#define SABER_CLIENT_CLIENT_WATCH_MANAGER_H_

#include <string>
#include <unordered_map>

#include "saber/service/watcher.h"

namespace saber {

class ClientWatchManager {
 public:
  explicit ClientWatchManager(Watcher* watcher = nullptr);
  ~ClientWatchManager();

  void AddDataWatch(const std::string& path, Watcher* watcher);
  void AddExistWatch(const std::string& path, Watcher* watcher);
  void AddChildWatch(const std::string& path, Watcher* watcher);

  WatcherSetPtr Trigger(const WatchedEvent& event);

 private:
  Watcher* watcher_;
  std::unordered_map<std::string, WatcherSetPtr> data_watches_;
  std::unordered_map<std::string, WatcherSetPtr> exist_watches_;
  std::unordered_map<std::string, WatcherSetPtr> child_watches_;

  // No copying allowed
  ClientWatchManager(const ClientWatchManager&);
  void operator=(const ClientWatchManager&);
};

}  // namespace saber

#endif  // SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
