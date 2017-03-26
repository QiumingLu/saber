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
    const CreateResponse&)> CreateCallback;

typedef std::function<void (int rc, const std::string& path, void* context
    )> DeleteCallback;

typedef std::function<void (int rc, const std::string& path, void* context, 
    const ExistsResponse&)> ExistsCallback;

typedef std::function<void (int rc, const std::string& path, void* context, 
    const GetDataResponse&)> GetDataCallback;

typedef std::function<void (int rc, const std::string& path, void* context, 
    const SetDataResponse&)> SetDataCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const GetACLResponse&)> GetACLCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const SetACLResponse&)> SetACLCallback;

typedef std::function<void (int rc, const std::string& path, void* context,
    const GetChildrenResponse&)> ChildrenCallback;

}

#endif  // SABER_CLIENT_CALLBACKS_H_
