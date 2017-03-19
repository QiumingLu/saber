// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "saber/server/server_watch_manager.h"

namespace saber {

ServerWatchManager::ServerWatchManager() {
}

ServerWatchManager::~ServerWatchManager() {
}

void ServerWatchManager::AddWatch(const std::string& path, Watcher* watcher) {
}

void ServerWatchManager::RemoveWatch(Watcher* watcher) {
}

void ServerWatchManager::TriggerWatcher(
    const std::string& path, std::set<Watcher*>* result) {
}

}  // namespace saber
