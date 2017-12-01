// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_CODING_H_
#define SABER_UTIL_CODING_H_

#include <voyager/util/coding.h>

namespace saber {

inline uint32_t DecodeFixed32(const char* p) {
  return voyager::DecodeFixed32(p);
}

inline uint64_t DecodeFixed64(const char* p) {
  return voyager::DecodeFixed64(p);
}

inline void PutFixed32(std::string* s, uint32_t value) {
  voyager::PutFixed32(s, value);
}

inline void PutFixed64(std::string* s, uint64_t value) {
  voyager::PutFixed64(s, value);
}

}  // namespace saber

#endif  // SABER_UTIL_CODING_H_
