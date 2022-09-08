// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_MAIN_TO_STRING_H_
#define SABER_MAIN_TO_STRING_H_

#include <string>

#include <saber/proto/saber.pb.h>

namespace saber {

std::string ToString(SessionState state);
std::string ToString(EventType type);
std::string ToString(const WatchedEvent& event);
std::string ToString(ResponseCode code);
std::string ToString(const Stat& stat);
std::string ToString(const CreateResponse& response);
std::string ToString(const DeleteResponse& response);
std::string ToString(const ExistsResponse& response);
std::string ToString(const GetDataResponse& response);
std::string ToString(const SetDataResponse& response);
std::string ToString(const GetChildrenResponse& response);

}  // namespace saber

#endif  // SABER_MAIN_TO_STRING_H_
