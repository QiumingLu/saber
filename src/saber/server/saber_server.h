// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_SERVER_SABER_SERVER_H_
#define SABER_SERVER_SABER_SERVER_H_

namespace saber {

class SaberServer {
 public:
  SaberServer();
  ~SaberServer();

  void Start();

 private:
  
  // No copying allowed
  SaberServer(const SaberServer&);
  void operator=(const SaberServer&);
};

}  // namespace saber

#endif  // SABER_SERVER_SABER_SERVER_H_
