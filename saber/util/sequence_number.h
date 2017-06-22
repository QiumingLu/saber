#ifndef SABER_UTIL_SEQUENCE_NUMBER_H_
#define SABER_UTIL_SEQUENCE_NUMBER_H_

#include "saber/util/mutex.h"
#include "saber/util/mutexlock.h"

namespace saber {

template <typename T>
class SequencenNumber {
 public:
  SequencenNumber(T max) : max_(max), num_(0) {}
  T GetNext() {
    MutexLock lock(&mutex_);
    if (num_ >= max_) {
      num_ = 0;
    }
    return num_++;
  }

 private:
  Mutex mutex_;
  T max_;
  T num_;

  // intentionally copy
};

}  // namespace saber

#endif  // SABER_UTIL_SEQUENCE_NUMBER_H_
