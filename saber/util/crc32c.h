// Copyright (c) 2016 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_CRC32C_H_
#define SABER_UTIL_CRC32C_H_

#include <stddef.h>
#include <stdint.h>

namespace saber {
namespace crc {

extern uint32_t crc32(uint32_t crc, const char* buf, size_t size);

}  // namespace crc
}  // namespace saber

#endif  // SABER_UTIL_CRC32C_H_
