#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <chrono>

#include <base/bind.h>
#include <base/callback.h>
#include <base/location.h>
#include <base/memory/ref_counted.h>
#include <base/synchronization/lock.h>
#include <base/optional.h>
#include <base/timer/timer.h>
#include <base/time/time.h>
#include <base/trace_event/trace_event.h>

namespace basis {

// ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess disables the check
// for rare pre-existing use cases where thread-safety was
// guaranteed through other means (e.g. explicit sequencing of calls across
// execution sequences when bouncing between threads in order).
class BASE_EXPORT ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess final {
 public:
#if DCHECK_IS_ON()
  ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess();
  ~ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess();
#else
  ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess() {}
  ~ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess() {}
#endif
};

// PrioritizedOnceTaskHeap allows for prioritization of stored tasks.
// It provides up to 2^32 priority levels (uint32_t).
// All tasks posted via the PrioritizedOnceTaskHeap
// will run in priority order.
//
/// \note Task with lowest priority will run first
/// (runs tasks by priority from 0 to +inf).
//
/// \note Stored task can be run only once (even if task is `RepeatingTask`).
/// If you want priority queue for periodic repeating tasks,
/// than you can use `::basis::PrioritizedList<::base::RepeatingClosure>`
//
// PrioritizedOnceTaskHeap is data type called a priority queue that was
// implemented using binary max heap (not a sorted structure) because we must
// remove only the object with the lowest priority
// (lowest priority element is always stored at the root).
//
/// \note See instead `PrioritizedRepeatingTaskList`
/// if you search for priority-based queue of periodic repeating callbacks.
//
class PrioritizedOnceTaskHeap
  : public ::base::RefCountedThreadSafe<PrioritizedOnceTaskHeap>
{
 public:
  /// \note `TaskId` grows with each stored task,
  /// but overflow allowed here.
  using TaskId = uint32_t;

  using TaskPriority = uint32_t;

  using RepeatingTask = ::base::RepeatingClosure;

  using OnceTask = ::base::OnceClosure;

  // Highest priority will run before other priority values.
  static constexpr TaskPriority kHighestPriority = 0;

  struct Job {
    Job(const ::base::Location& from_here,
        OnceTask&& task,
        TaskPriority priority,
        TaskId current_task_count);
    Job();
    ~Job();

    Job(Job&& other);
    Job& operator=(Job&& other);

    ::base::Location from_here;
    OnceTask task;
    TaskPriority priority = 0;
    // task id based on `max_task_count_` at the moment of construction
    TaskId task_id = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Job);
  };

  struct JobComparer {
    bool operator()(const Job& left, const Job& right) {
      if (left.priority == right.priority)
        return left.task_id > right.task_id;
      return left.priority > right.priority;
    }
  };

  PrioritizedOnceTaskHeap(bool with_thread_locking = false
    , std::vector<Job> task_job_heap = std::vector<Job>{});

  // Task runs at |priority|.
  // Priority 0 is the highest priority and will run before other
  // priority values.
  // Multiple tasks with the same |priority| value are run in
  // order of posting.
  void ScheduleTask(
    const ::base::Location& from_here
    /// \note RepeatingTask will be converted to OnceTask
    , RepeatingTask task
    , TaskPriority priority);

  void ScheduleTask(
    const ::base::Location& from_here
    , OnceTask&& task
    , TaskPriority priority);

  void RunAndPopLargestTask();

  void RunAllTasks();

  size_t size() noexcept;

 private:
  friend class ::base::RefCountedThreadSafe<PrioritizedOnceTaskHeap>;

  /// \note from |base::RefCounted| docs:
  /// You should always make your destructor non-public,
  /// to avoid any code deleting
  /// the object accidently while there are references to it.
  ~PrioritizedOnceTaskHeap();

#if DCHECK_IS_ON()
  bool CalledOnValidSequenceOrUsesLocks() const;
#endif

  void AcquireLockIfNeeded() noexcept;

  void ReleaseLockIfNeeded() noexcept;

  void AssertAcquiredLockIfNeeded() noexcept;

  // Returns the index of the left child based on node at position index.
  /// \note We use STL, so nodes of heap have two children (leftChildIndex and rightChildIndex).
  /// \note Do not forget to check that index is not out-of-bounds.
  size_t leftChildIndex(size_t index)
  {
    return (index + 1) * 2 - 1;
  }

  // Returns the index of the right child based on node at position index.
  /// \note We use STL, so nodes of heap have two children (leftChildIndex and rightChildIndex).
  /// \note Do not forget to check that index is not out-of-bounds.
  size_t rightChildIndex(size_t index)
  {
    return (index + 1) * 2;
  }

  const bool use_thread_locking_;

  // Accessed on both task_runner_ and the reply task runner.
  // |std::priority_queue| does not support move-only types,
  // so we use |std::vector|.
  //
  // Representation of heap using `std::vector` is more efficient than
  // having nodes pointing to each other for several reasons:
  // * there is only one dynamic allocation for all the heap, and not one per node,
  // * there is no pointers to children, so no space needed for them,
  // * the contiguous layout of the structure makes it more cache-friendly.
  std::vector<Job> task_job_heap_;

  // Used to preserve order of jobs of equal priority.
  // Stores max. i.e. never decreases.
  /// \note This can overflow and cause periodic priority inversion.
  /// This should be infrequent enough to be of negligible impact
  /// i.e. overflow allowed here.
  TaskId max_task_count_ = 0;

  base::Lock task_job_heap_lock_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PrioritizedOnceTaskHeap);
};

}  // namespace basis
