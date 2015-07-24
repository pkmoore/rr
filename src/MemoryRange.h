/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#ifndef RR_MEMORY_RANGE_H_
#define RR_MEMORY_RANGE_H_

#include "remote_ptr.h"

/**
 * Range of memory addresses that can be used as a std::map key.
 */
class MemoryRange {
public:
  MemoryRange(remote_ptr<void> addr, size_t num_bytes)
      : start_(addr), end_(addr + num_bytes) {
    assert(start_ <= end_);
  }
  MemoryRange(remote_ptr<void> addr, remote_ptr<void> end)
      : start_(addr), end_(end) {
    assert(start_ <= end);
  }
  MemoryRange(const MemoryRange&) = default;
  MemoryRange& operator=(const MemoryRange&) = default;

  bool operator==(const MemoryRange& o) const {
    return start_ == o.start_ && end_ == o.end_;
  }
  bool operator<(const MemoryRange& o) const {
    return start_ != o.start_ ? start_ < o.start_ : end_ < o.end_;
  }

  bool intersects(const MemoryRange& other) const {
    remote_ptr<void> s = std::max(start_, other.start_);
    remote_ptr<void> e = std::min(end_, other.end_);
    return s < e;
  }

  remote_ptr<void> start() const { return start_; }
  remote_ptr<void> end() const { return end_; }
  size_t size() const { return end_ - start_; }

private:
  remote_ptr<void> start_;
  remote_ptr<void> end_;
};

#endif /* RR_MEMORY_RANGE_H_ */
