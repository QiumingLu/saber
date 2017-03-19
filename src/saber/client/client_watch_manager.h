// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
#define SABER_CLIENT_CLIENT_WATCH_MANAGER_H_

namespace saber {

class ClientWatchManager {
 public:
  ClientWatchManager();
  ~ClientWatchManager();

 private:
  // No copying allowed
  ClientWatchManager(const ClientWatchManager&);
  void operator=(const ClientWatchManager&);
};

}  // namespace saber

#endif  // SABER_CLIENT_CLIENT_WATCH_MANAGER_H_
