// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef SABER_UTIL_HASH_H_
#define SABER_UTIL_HASH_H_

#include <stdint.h>
#include <string.h>

namespace saber {

extern uint32_t Hash(const char* data, size_t n, uint32_t seed);

}

#endif  // SABER_UTIL_HASH_H_
