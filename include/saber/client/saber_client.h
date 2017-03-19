// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_SABER_CLIENT_H_
#define SABER_CLIENT_SABER_CLIENT_H_

#include "saber/client/callbacks.h"

namespace saber {

class SaberClient {
 public:
  SaberClient();
  ~SaberClient();

  void Start();
 
 private:
  // No copying allowed
  SaberClient(const SaberClient&);
  void operator=(const SaberClient&);
};

}  // namespace saber

#endif  // SABER_CLIENT_SABER_CLIENT_H_
