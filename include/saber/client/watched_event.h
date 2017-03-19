// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_WATCHED_EVENT_H_
#define SABER_CLIENT_WATCHED_EVENT_H_

namespace saber {

class WatchedEvent {
 public:
  WatchedEvent();
  ~WatchedEvent();

 private:
  // No copying allowed
  WatchedEvent(const WatchedEvent&);
  void operator=(const WatchedEvent&);
};

}  // namespace saber

#endif  // SABER_CLIENT_WATCHED_EVENT_H_
