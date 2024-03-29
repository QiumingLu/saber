// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_CLIENT_CALLBACKS_H_
#define SABER_CLIENT_CALLBACKS_H_

#include <functional>
#include <string>

#include "saber/proto/saber.pb.h"

namespace saber {

typedef std::function<void(const std::string& path, void* context,
                           const CreateResponse&)>
    CreateCallback;

typedef std::function<void(const std::string& path, void* context,
                           const DeleteResponse&)>
    DeleteCallback;

typedef std::function<void(const std::string& path, void* context,
                           const ExistsResponse&)>
    ExistsCallback;

typedef std::function<void(const std::string& path, void* context,
                           const GetDataResponse&)>
    GetDataCallback;

typedef std::function<void(const std::string& path, void* context,
                           const SetDataResponse&)>
    SetDataCallback;

typedef std::function<void(const std::string& path, void* context,
                           const GetChildrenResponse&)>
    GetChildrenCallback;

}  // namespace saber

#endif  // SABER_CLIENT_CALLBACKS_H_
