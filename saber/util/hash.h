// Copyright (c) 2016 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef SABER_UTIL_HASH_H_
#define SABER_UTIL_HASH_H_

#include <stdint.h>
#include <string.h>

namespace saber {

extern uint32_t Hash(const char* data, size_t n, uint32_t seed);

}  // namespace saber

#endif  // SABER_UTIL_HASH_H_
