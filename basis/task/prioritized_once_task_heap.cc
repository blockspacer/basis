#include "basis/task/prioritized_once_task_heap.hpp" // IWYU pragma: associated

#include <algorithm>

#include <base/bind.h>
#include <base/rvalue_cast.h>

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
  TaskVariant task,
  TaskPriority priority,
  TaskId current_task_count)
  : from_here(from_here),
    task(base::rvalue_cast(task)),
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
  , task_job_heap_(base::rvalue_cast(task_job_heap))
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

  Job job(
    from_here
    , base::rvalue_cast(task)
    , priority
    , max_task_count_++);

  {
    AcquireLockIfNeeded();
    task_job_heap_.push_back(base::rvalue_cast(job));
    // Add element at the end of the heap.
    // If that is not its right position,
    // than it bubble up its way through the heap.
    std::push_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());
    ReleaseLockIfNeeded();
  }
}

std::vector<PrioritizedOnceTaskHeap::Job>
  PrioritizedOnceTaskHeap::extractSubHeap(size_t subRootIndex)
{
  DCHECK(CalledOnValidSequenceOrUsesLocks());

  std::vector<size_t> subHeapIndices;

  /// \todo refactor without using queue
  std::queue<size_t> currentIndices;

  currentIndices.push(subRootIndex);
  subHeapIndices.push_back(subRootIndex);

  std::vector<Job> subHeap;

  {
    AcquireLockIfNeeded();

    /// \note runs under lock to make sure that `task_job_heap_.size()`
    /// does not change until we fill `subHeap`.
    while (!currentIndices.empty())
    {
      size_t index = currentIndices.front();
      size_t left_child_index = leftChildIndex(index);
      if (left_child_index < task_job_heap_.size())
      {
        currentIndices.push(left_child_index);
        subHeapIndices.push_back(left_child_index);
      }
      size_t right_child_index = rightChildIndex(index);
      if (right_child_index < task_job_heap_.size())
      {
        currentIndices.push(right_child_index);
        subHeapIndices.push_back(right_child_index);
      }
      currentIndices.pop();
    }

    std::transform(subHeapIndices.begin()
      , subHeapIndices.end()
      , std::back_inserter(subHeap)
      , [
          this
        ](
          size_t index
        ) -> Job&& {
          DCHECK(index < task_job_heap_.size());
          /// \note moves elements out from original heap,
          /// take care of data validity and test under ASAN.
          return base::rvalue_cast(task_job_heap_[index]);
        });
    ReleaseLockIfNeeded();
  }

  return subHeap;
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
    , base::rvalue_cast(task)
    , priority
    , max_task_count_++);

  {
    AcquireLockIfNeeded();
    task_job_heap_.push_back(base::rvalue_cast(job));
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

namespace {

void runRepeatingTask(PrioritizedOnceTaskHeap::RepeatingTask task
  , const base::Location& from_here)
{
  DCHECK(task)
    << "Unexpected repeating task. Location: "
    << from_here.ToString();

  task.Run();
}

void runOnceTask(PrioritizedOnceTaskHeap::OnceTask&& task
  , const base::Location& from_here)
{
  DCHECK(task)
    << "Unexpected once task. Location: "
    << from_here.ToString();

  base::rvalue_cast(task).Run();
}

} // namespace

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

    Job& job = task_job_heap_.back();

    const bool is_repeating_task{std::holds_alternative<RepeatingTask>(job.task)};

    if(is_repeating_task) {
      // take care of data validity because Job can be moved out
      if (std::get<RepeatingTask>(job.task)) {
        // we can copy repeating task to avoid locking durung `task.run()`
        closureWithoutLock = base::BindOnce(
          &runRepeatingTask
          , std::get<RepeatingTask>(job.task)
          , job.from_here
        );
      }
    } else {
      DCHECK(std::holds_alternative<OnceTask>(job.task));
      // take care of data validity because Job can be moved out
      if (std::get<OnceTask>(job.task)) {
        // we can move once task to avoid locking durung `task.run()`
        closureWithoutLock = base::BindOnce(
          &runOnceTask
          , base::rvalue_cast(std::get<OnceTask>(job.task))
          , job.from_here
        );
      }
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
    base::rvalue_cast(closureWithoutLock).Run();
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
