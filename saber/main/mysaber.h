// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_MAIN_MYSABER_H_
#define SABER_MAIN_MYSABER_H_

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include <saber/client/client_options.h>
#include <saber/client/saber.h>
#include <saber/util/runloop.h>
#include <voyager/core/bg_eventloop.h>
#include <voyager/util/string_util.h>

#include "saber/main/default_watcher.h"

namespace saber {

class MySaber {
 public:
  MySaber(RunLoop* loop, const ClientOptions& options);
  ~MySaber();

  void Start();

 private:
  enum RequestType {
    kCreate = 0,
    kDelete = 1,
    kExists = 2,
    kGetData = 3,
    kSetData = 4,
    kGetACL = 5,
    kSetACL = 6,
    kGetChildren = 7,
  };

  void GetLine();
  bool NewRequest(const std::string& s);
  void Create();
  void OnCreateReply(const std::string& path, void* context,
                     const CreateResponse& response);
  void Delete();
  void OnDeleteReply(const std::string& path, void* context,
                     const DeleteResponse& response);
  void Exists();
  void OnExistsReply(const std::string& path, void* context,
                     const ExistsResponse& response);
  void GetData();
  void OnGetDataReply(const std::string& path, void* context,
                      const GetDataResponse& response);
  void SetData();
  void OnSetDataReply(const std::string& path, void* context,
                      const SetDataResponse& response);
  void GetACL();
  void OnGetACLReply(const std::string& path, void* context,
                     const GetACLResponse& response);
  void SetACL();
  void OnSetACLReply(const std::string& path, void* context,
                     const SetACLResponse& response);
  void GetChildren();
  void OnGetChildrenReply(const std::string& path, void* context,
                          const GetChildrenResponse& response);

  RunLoop* loop_;
  DefaultWatcher watcher_;
  voyager::BGEventLoop thread_;
  Saber saber_;

  // No copying allowed
  MySaber(const MySaber&);
  void operator=(const MySaber&);
};

}  // namespace saber

#endif  // SABER_MAIN_MY_SABER_H_
