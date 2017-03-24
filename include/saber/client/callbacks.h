// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CALLBACKS_H_
#define SABER_CLIENT_CALLBACKS_H_

#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "saber/proto/saber.pb.h"

namespace saber {

typedef std::function<void (int rc, const std::string& path, void* context, 
    const Stat& stat)> StatCallback;

typedef std::function<void (int rc, const std::string& path, void* context, 
    const std::string& data, const Stat& stat)> DataCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::vector<std::unique_ptr<ACL> >& v, const Stat& stat)> ACLCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::vector<std::string>& children)> ChildrenCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::vector<std::string>& children, const Stat& stat)> Children2Callback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::string& name, const Stat& stat)> Create2Callback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::string& name)> StringCallback;

typedef std::function<void (int rc, const std::string& path, void* context
    )> VoidCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const std::vector<std::string>& op)> MultiCallback;

}

#endif  // SABER_CLIENT_CALLBACKS_H_
