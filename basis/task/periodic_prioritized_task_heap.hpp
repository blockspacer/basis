#pragma once

#include <basis/timer/IntervalTimer.hpp>

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

// PeriodicPrioritizedTaskHeap allows for prioritization of posted tasks.
// It provides up to 2^32 priority levels.
// All tasks posted via the PeriodicPrioritizedTaskHeap
// will run in priority order.
class PeriodicPrioritizedTaskHeap
  : public ::base::RefCountedThreadSafe<PeriodicPrioritizedTaskHeap>
{
 public:
  // Highest priority will run before other priority values.
  static constexpr uint32_t kHighestPriority = 0;

  PeriodicPrioritizedTaskHeap();

  typedef ::base::RepeatingCallback<
      void(
        const std::chrono::nanoseconds& current_frame_elapsed_dt
        , const std::chrono::nanoseconds& last_call_elapsed_dt
        , bool* stop_repeating_task)
    > Callback;

  // Task runs at |priority|.
  // Priority 0 is the highest priority and will run before other
  // priority values.
  // Multiple tasks with the same |priority| value are run in
  // order of posting.
  void ScheduleTask(
    const ::base::Location& from_here
    , Callback task
    , uint32_t priority
    , const std::chrono::nanoseconds& interval);

  void RunLargestTask(
    const std::chrono::nanoseconds& current_frame_elapsed_dt);

  void RunAllTasks(
    const std::chrono::nanoseconds& current_frame_elapsed_dt);

 private:
  friend class ::base::RefCountedThreadSafe<PeriodicPrioritizedTaskHeap>;

  /// \note from |base::RefCounted| docs:
  /// You should always make your destructor non-public,
  /// to avoid any code deleting
  /// the object accidently while there are references to it.
  ~PeriodicPrioritizedTaskHeap();

  struct Job {
    Job(const ::base::Location& from_here,
        Callback task,
        uint32_t priority,
        uint32_t task_count,
        const std::chrono::nanoseconds& interval);
    Job();
    ~Job();

    Job(Job&& other);
    Job& operator=(Job&& other);

    ::base::Location from_here;
    Callback task;
    uint32_t priority = 0;
    uint32_t task_count = 0;
    uint32_t iteration = 0;
    ::basis::IntervalTimer timer;

   private:
    DISALLOW_COPY_AND_ASSIGN(Job);
  };

  struct JobComparer {
    bool operator()(const Job& left, const Job& right) {
      if (left.priority == right.priority)
        return left.task_count > right.task_count;
      return left.priority > right.priority;
    }
  };

  // Accessed on both task_runner_ and the reply task runner.
  // |std::priority_queue| does not support move-only types,
  // so we use |std::vector|.
  std::vector<Job> task_job_heap_;

  // Used to preserve order of jobs of equal priority.
  /// \note This can overflow and cause periodic priority inversion.
  /// This should be infrequent enough to be of negligible impact.
  uint32_t task_count_ = 0;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PeriodicPrioritizedTaskHeap);
};

}  // namespace basis
