// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CALLBACKS_H_
#define SABER_CLIENT_CALLBACKS_H_

#include <functional>

namespace saber {

typedef std::function<void ()> StatCallback;
typedef std::function<void ()> DataCallback;
typedef std::function<void ()> ACLCallback;
typedef std::function<void ()> ChildrenCallback;
typedef std::function<void ()> Children2Callback;
typedef std::function<void ()> Create2Callback;
typedef std::function<void ()> StringCallback;
typedef std::function<void ()> VoidCallback;
typedef std::function<void ()> MultiCallback;

}

#endif  // SABER_CLIENT_CALLBACKS_H_
