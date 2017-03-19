// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_NET_MESSAGER_H_
#define SABER_NET_MESSAGER_H_

namespace saber {

class Messager {
 public:
  Messager();
  ~Messager();

 private:
  Messager(const Messager&);
  void operator=(const Messager&);
};

}  // namespace saber

#endif  // SABER_NET_MESSAGER_H_
