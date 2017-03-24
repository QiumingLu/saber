// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVICE_WATCH_MANAGER_H_
#define SABER_SERVICE_WATCH_MANAGER_H_

namespace saber {

class WatchManager {
 public:
  WatchManager();
  virtual ~WatchManager();

 private:
  // No copying allowed
  WatchManager(const WatchManager&);
  void operator=(const WatchManager&);
};

}  // namespace saber

#endif  // SABER_SERVICE_WATCH_MANAGER_H_
