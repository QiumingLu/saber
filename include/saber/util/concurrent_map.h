// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_UTIL_CONCURRENT_MAP_H_
#define SABER_UTIL_CONCURRENT_MAP_H_

#include <utility>
#include <unordered_map>

#include "saber/util/mutex.h"
#include "saber/util/mutexlock.h"

namespace saber {

static const int kNumShardBits = 4;
static const int kNumShards = 1 << kNumShardBits;

template <typename K, typename V>
class HashMap {
 public:
  HashMap() : mu_() { }

  void insert(const K& key, const V& value) {
    MutexLock lock(&mu_);
    map_[key] = value;
  }

  void insert(const K& key, V&& value) {
    MutexLock lock(&mu_);
    map_[key] = std::move(value);
  }

  void erase(const K& key) {
    MutexLock lock(&mu_);
    map_.erase(key);
  }

  size_t size() const {
    MutexLock lock(&mu_);
    return map_.size();
  }

 private:
  mutable Mutex mu_;
  typename std::unordered_map<K, V> map_;

  // No copying allowed
  HashMap(const HashMap&);
  void operator=(const HashMap&);
};

template <typename K, typename V>
class ConcurrentMap {
 public:
  ConcurrentMap() { }

  void insert(const K& key, const V& value) {
    const size_t h = Hash(key);
    shard_[Shard(h)].insert(key, value);
  }

  void insert(const K& key, V&& value) {
    const size_t h = Hash(key);
    shard_[Shard(h)].insert(key, std::move(value));
  }

  void erase(const K& key) {
    const size_t h = Hash(key);
    shard_[Shard(h)].erase(key);
  }

  size_t size() const {
    size_t total = 0;
    for (int s = 0; s < kNumShards; ++s) {
      total += shard_[s].size();
    }
    return total;
  }

 private:
  static inline size_t Hash(const K& key) {
    typename std::hash<K> h;
    return h(key);
  }

  static inline size_t Shard(size_t h) {
    return (h & (kNumShards - 1));
  }

  HashMap<K, V> shard_[kNumShards];

  // No copying allowed
  ConcurrentMap(const ConcurrentMap&);
  void operator=(const ConcurrentMap&);
};

}  // namespace saber

#endif  // SABER_UTIL_CONCURRENT_MAP_H_
