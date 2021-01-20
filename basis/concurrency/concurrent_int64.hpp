// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
#pragma once

#include <base/logging.h>
#include <base/atomicops.h>
#include <base/threading/thread_local.h>
#include <base/threading/platform_thread.h>
#include <base/lazy_instance.h>
#include <base/compiler_specific.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifndef ARCH_CPU_64_BITS
#error "ARCH NOT SUPPORTED"
#endif // ARCH_CPU_64_BITS

namespace basis {

class Concurrent64;

namespace concurrent64 {
namespace internal {

struct HashCode {
 public:
  HashCode();
  uint64_t code_;
};

#define ATOMIC_INT_SIZE sizeof(base::subtle::Atomic64)
// Padded POD container for Atomic64. This prevents false sharing of cache lines.
class Cell {
 public:
  Cell();
  inline bool CompareAndSet(int64_t cmp, int64_t value) {
    return base::subtle::NoBarrier_CompareAndSwap(&value_, cmp, value) == cmp;
  }

  // Padding advice from Herb Sutter:
  // http://www.drdobbs.com/parallel/eliminate-false-sharing/217500206?pgno=4
  base::subtle::Atomic64 value_;
  char pad[BASE_CACHELINE_SIZE > ATOMIC_INT_SIZE ?
           BASE_CACHELINE_SIZE - ATOMIC_INT_SIZE : 1];

  DISALLOW_COPY_AND_ASSIGN(Cell);
} BASE_CACHELINE_ALIGNED;
#undef ATOMIC_INT_SIZE

} // namespace internal
} // namespace concurrent64

// This set of classes is heavily derived from JSR166e, released into the public domain
// by Doug Lea and the other authors.
//
// See: http://gee.cs.oswego.edu/cgi-bin/viewcvs.cgi/jsr166/src/jsr166e/Striped64.java?view=co
// See: http://gee.cs.oswego.edu/cgi-bin/viewcvs.cgi/jsr166/src/jsr166e/LongAdder.java?view=co
// See: https://docs.microsoft.com/ru-ru/dotnet/api/java.util.concurrent.atomic.striped64?view=xamarin-android-sdk-9
//
// The Concurrent64 (aka Striped64) and LongAdder implementations here are simplified versions of what's present in
// JSR166e. However, the core ideas remain the same.
//
// Updating a single AtomicInteger in a multi-threaded environment can be quite slow:
//
//   1. False sharing of cache lines with other counters.
//   2. Cache line bouncing from high update rates, especially with many cores.
//
// These two problems are addressed by Striped64. When there is no contention,
// it uses CAS (compare and swap) on a single base counter to store updates.
// However, when Striped64 detects contention
// (via a failed CAS operation), it will allocate a small, fixed size hashtable of Cells.
// A Cell is a simple POD that pads out an AtomicInt to 64 bytes to prevent
// sharing a cache line.
//
// Reading the value of a Striped64 requires traversing the hashtable to calculate the true sum.
//
// Each updating thread uses a thread-local hashcode to determine its Cell in the hashtable.
// If a thread fails to CAS its hashed Cell, it will do a lightweight rehash operation to try
// and find an uncontended bucket. Because the hashcode is thread-local, this rehash affects all
// Striped64's accessed by the thread. This is good, since contention on one Striped64 is
// indicative of contention elsewhere too.
//
// The hashtable is statically sized to the nearest power of 2 greater than or equal to the
// number of CPUs. This is sufficient, since this guarantees the existence of a perfect hash
// function. Due to the random rehashing, the threads should eventually converge to this function.
// In practice, this scheme has shown to be sufficient.
//
// The biggest simplification of this implementation compared to JSR166e is that we do not
// dynamically grow the table, instead immediately allocating it to the full size.
// We also do not lazily allocate each Cell, instead allocating the entire array at once.
// This means we waste some additional memory in low contention scenarios, and initial allocation
// will also be slower. Some of the micro-optimizations were also elided for readability.
// See http://gee.cs.oswego.edu/cgi-bin/viewcvs.cgi/jsr166/src/jsr166e/LongAdder.java
class Concurrent64 {
 public:
  Concurrent64();
  virtual ~Concurrent64();

 protected:

  enum Rehash {
    kRehash,
    kNoRehash
  };

  // CAS the base field.
  bool CasBase(int64_t cmp, int64_t val) { return base_.CompareAndSet(cmp, val); }

  // CAS the busy field from 0 to 1 to acquire the lock.
  bool CasBusy() {
    return base::subtle::NoBarrier_CompareAndSwap(&busy_, 0, 1) == 0;
  }

  // Computes the function of the current and new value. Used in RetryUpdate.
  virtual int64_t Fn(int64_t current_value, int64_t new_value) = 0;

  // Handles cases of updates involving initialization, resizing, creating new Cells, and/or
  // contention. See above for further explanation.
  void RetryUpdate(int64_t x, Rehash to_rehash);

  // Sets base and all cells to the given value.
  void InternalReset(int64_t initialValue);

  // Base value, used mainly when there is no contention, but also as a fallback during
  // table initialization races. Updated via CAS.
  concurrent64::internal::Cell base_;

  // CAS lock used when resizing and/or creating cells.
  base::subtle::Atomic32 busy_;

  // Backing buffer for cells_, used for alignment.
  void* cell_buffer_;

  // Table of cells. When non-null, size is the nearest power of 2 >= NCPU.
  concurrent64::internal::Cell* cells_;
  int32_t num_cells_;

  // Static hash code per-thread. Shared across all instances to limit thread-local pollution.
  // Also, if a thread hits a collision on one Concurrent64, it's also likely to collide on
  // other Concurrent64-s too.
  static base::LazyInstance<
    base::ThreadLocalOwnedPointer<concurrent64::internal::HashCode>
  >::Leaky hashcode_;

 private:

  // Number of CPUs, to place bound on table size.
  static const uint32_t kNumCpus;

};

// A 64-bit number optimized for high-volume concurrent updates.
// See Concurrent64 for a longer explanation of the inner workings.
// See: http://gee.cs.oswego.edu/cgi-bin/viewcvs.cgi/jsr166/src/jsr166e/LongAdder.java?view=co
// See: https://docs.oracle.com/javase/8/docs/api/java/util/concurrent/atomic/LongAdder.html
class Concurrent64LongAdder : Concurrent64 {
 public:
  Concurrent64LongAdder() {}
  void IncrementBy(int64_t x);
  void Increment() { IncrementBy(1); }
  void Decrement() { IncrementBy(-1); }

  // Returns the current value.
  // Note this is not an atomic snapshot in the presence of concurrent updates.
  int64_t Value() const;

  // Resets the counter state to zero.
  void Reset() { InternalReset(0); }

 private:
  int64_t Fn(int64_t current_value, int64_t new_value) override {
    return current_value + new_value;
  }

  DISALLOW_COPY_AND_ASSIGN(Concurrent64LongAdder);
};

} // namespace basis
