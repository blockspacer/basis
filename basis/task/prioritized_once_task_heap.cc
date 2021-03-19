#include "basis/task/prioritized_once_task_heap.hpp" // IWYU pragma: associated

#include <algorithm>

#include <base/bind.h>
#include <basic/rvalue_cast.h>

#include <queue>

namespace {

#if DCHECK_IS_ON()
std::atomic_int g_cross_thread_prioritized_task_heap_access_allow_count(0);
#endif

} // namespace

namespace basis {

#if DCHECK_IS_ON()
ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess::ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess() {
  ++g_cross_thread_prioritized_task_heap_access_allow_count;
}

ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess::~ScopedAllowCrossThreadPrioritizedOnceTaskHeapAccess() {
  --g_cross_thread_prioritized_task_heap_access_allow_count;
}
#endif

#if DCHECK_IS_ON()
bool PrioritizedOnceTaskHeap::CalledOnValidSequenceOrUsesLocks() const {
  return sequence_checker_.CalledOnValidSequence()
         || g_cross_thread_prioritized_task_heap_access_allow_count.load() != 0
         || use_thread_locking_;
}
#endif

PrioritizedOnceTaskHeap::Job::Job(
  const ::base::Location& from_here,
  OnceTask&& task,
  TaskPriority priority,
  TaskId current_task_count)
  : from_here(from_here),
    task(RVALUE_CAST(task)),
    priority(priority),
    task_id(current_task_count)
{
}

PrioritizedOnceTaskHeap::Job::Job() {}

PrioritizedOnceTaskHeap::Job::~Job() = default;
PrioritizedOnceTaskHeap::Job::Job(Job&& other) = default;
PrioritizedOnceTaskHeap::Job&
  PrioritizedOnceTaskHeap::Job::operator=(Job&& other) =
    default;

PrioritizedOnceTaskHeap::PrioritizedOnceTaskHeap(bool with_thread_locking
  , std::vector<Job> task_job_heap)
  : use_thread_locking_(with_thread_locking)
  , task_job_heap_(RVALUE_CAST(task_job_heap))
{
  SEQUENCE_CHECKER(sequence_checker_);
}

void PrioritizedOnceTaskHeap::ScheduleTask(
  const ::base::Location& from_here
  , RepeatingTask task
  , TaskPriority priority)
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  DCHECK(task)
    << "Unexpected repeating task. Location: "
    << from_here.ToString();

  // RepeatingCallback should be convertible to OnceCallback.
  OnceTask onceTask = task;

  Job job(
    from_here
    , RVALUE_CAST(onceTask)
    , priority
    , max_task_count_++);

  DCHECK(job.task)
    << "Unexpected once task. Location: "
    << from_here.ToString();

  {
    AcquireLockIfNeeded();
    task_job_heap_.push_back(RVALUE_CAST(job));
    // Add element at the end of the heap.
    // If that is not its right position,
    // than it bubble up its way through the heap.
    std::push_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());
    ReleaseLockIfNeeded();
  }
}

void PrioritizedOnceTaskHeap::AcquireLockIfNeeded() noexcept
{
  if(use_thread_locking_) {
    task_job_heap_lock_.Acquire();
  }
}

void PrioritizedOnceTaskHeap::ReleaseLockIfNeeded() noexcept
{
  if(use_thread_locking_) {
    AssertAcquiredLockIfNeeded();
    task_job_heap_lock_.Release();
  }
}

void PrioritizedOnceTaskHeap::AssertAcquiredLockIfNeeded() noexcept
{
  if(use_thread_locking_) {
    /// \note asserts only in debug mode
    task_job_heap_lock_.AssertAcquired();
  }
}

void PrioritizedOnceTaskHeap::ScheduleTask(
  const ::base::Location& from_here
  , OnceTask&& task
  , TaskPriority priority)
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  DCHECK(task)
    << "Unexpected once task. Location: "
    << from_here.ToString();

  Job job(
    from_here
    , RVALUE_CAST(task)
    , priority
    , max_task_count_++);

  {
    AcquireLockIfNeeded();
    task_job_heap_.push_back(RVALUE_CAST(job));
    // Add element at the end of the heap.
    // If that is not its right position,
    // than it bubble up its way through the heap.
    std::push_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());
    ReleaseLockIfNeeded();
  }
}

PrioritizedOnceTaskHeap::~PrioritizedOnceTaskHeap()
{
  /// \note destruction must be always performed
  /// on same thread that used during construction
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void PrioritizedOnceTaskHeap::RunAllTasks()
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  const size_t cached_size = size();
  for(size_t i = 0; i < cached_size; i++) {
    RunAndPopLargestTask();
  }

  DCHECK_EQ(size(), 0u);
}

void PrioritizedOnceTaskHeap::RunAndPopLargestTask()
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  base::OnceClosure closureWithoutLock;

  auto retrieveTask = [&]() {
    DCHECK(CalledOnValidSequenceOrUsesLocks());
    AssertAcquiredLockIfNeeded();

    // We can only remove one element, the root heap element
    // i.e. next job to run.
    // Moves the largest to the end
    // and rearranges the other elements into a heap.
    std::pop_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());

    // take care of data validity because Job can be moved out
    {
      Job& job = task_job_heap_.back();

      DCHECK(job.task)
        << "Unexpected once task. Location: "
        << FROM_HERE.ToString();

      // we can move once task to avoid locking durung `task.run()`
      closureWithoutLock = RVALUE_CAST(job.task);
    }

    // removes the largest element (previous root)
    task_job_heap_.pop_back();

    DCHECK(std::is_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer()))
      << "collection must be structured as a max heap";
  };

  {
    AcquireLockIfNeeded();
    retrieveTask();
    ReleaseLockIfNeeded();
  }

  // Task may be heavy, so make sure it requires no locks.
  // Thats why we copy or move data from `task_job_heap_` .
  if(closureWithoutLock) {
    RVALUE_CAST(closureWithoutLock).Run();
  }
}

size_t PrioritizedOnceTaskHeap::size() noexcept
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  size_t size{0u};

  {
    AcquireLockIfNeeded();
    size = task_job_heap_.size();
    ReleaseLockIfNeeded();
  }

  return size;
}

}  // namespace basis
