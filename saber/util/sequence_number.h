// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_SEQUENCE_NUMBER_H_
#define SABER_UTIL_SEQUENCE_NUMBER_H_

#include <mutex>

namespace saber {

template <typename T>
class SequenceNumber {
 public:
  SequenceNumber(T max) : max_(max), num_(0) {}
  T GetNext() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (num_ >= max_) {
      num_ = 0;
    }
    return num_++;
  }

 private:
  std::mutex mutex_;
  T max_;
  T num_;

  // No copying allowed
  SequenceNumber(const SequenceNumber&);
  void operator=(const SequenceNumber&);
};

}  // namespace saber

#endif  // SABER_UTIL_SEQUENCE_NUMBER_H_
