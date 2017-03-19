// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_PAXOS_COMMITTER_H_
#define SABER_PAXOS_COMMITTER_H_

namespace saber {

class Committer {
 public:
  Committer();
  ~Committer();

 private:
  // No copying allowed
  Committer(const Committer&);
  void operator=(const Committer&);
};

}  // namespace saber

#endif  // SABER_PAXOS_COMMITTER_H_
