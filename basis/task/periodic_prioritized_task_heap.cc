#include "basis/task/periodic_prioritized_task_heap.hpp" // IWYU pragma: associated

#include <algorithm>

#include <base/bind.h>

namespace basis {

PeriodicPrioritizedTaskHeap::Job::Job(
  const base::Location& from_here,
  Callback task,
  uint32_t priority,
  uint32_t task_count,
  const std::chrono::nanoseconds& interval)
  : from_here(from_here),
    task(std::move(task)),
    priority(priority),
    task_count(task_count),
    timer(interval)
{
}

PeriodicPrioritizedTaskHeap::Job::Job() {}

PeriodicPrioritizedTaskHeap::Job::~Job() = default;
PeriodicPrioritizedTaskHeap::Job::Job(Job&& other) = default;
PeriodicPrioritizedTaskHeap::Job&
  PeriodicPrioritizedTaskHeap::Job::operator=(Job&& other) =
    default;

PeriodicPrioritizedTaskHeap::PeriodicPrioritizedTaskHeap()
{
  SEQUENCE_CHECKER(sequence_checker_);
}

void PeriodicPrioritizedTaskHeap::ScheduleTask(
  const base::Location& from_here
  , Callback task
  , uint32_t priority
  , const std::chrono::nanoseconds& interval)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(task)
    << "Unexpected Callback. Location: "
    << from_here.ToString();

  DCHECK(interval.count() > 0)
    << "Unexpected interval. Location: "
    << from_here.ToString();

  Job job(
    from_here
    , std::move(task)
    , priority
    , task_count_++
    , interval);

  task_job_heap_.push_back(std::move(job));

  std::push_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());
}

PeriodicPrioritizedTaskHeap::~PeriodicPrioritizedTaskHeap()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void PeriodicPrioritizedTaskHeap::RunAllTasks(
  const std::chrono::nanoseconds& current_frame_elapsed_dt)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for(size_t i = 0; i < task_job_heap_.size(); i++) {
    RunLargestTask(current_frame_elapsed_dt);
  }
}

void PeriodicPrioritizedTaskHeap::RunLargestTask(
  const std::chrono::nanoseconds& current_frame_elapsed_dt)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Find the next job to run.
  // Moves the largest to the end
  std::pop_heap(task_job_heap_.begin(), task_job_heap_.end(), JobComparer());

  bool need_stop_repeating_task = false;

  Job& job = task_job_heap_.back();

  DCHECK(job.timer.GetInterval().count() > 0)
    << "Unexpected interval. Location: "
    << job.from_here.ToString();

  DCHECK(job.task)
    << "Unexpected Callback. Location: "
    << job.from_here.ToString();

  job.timer.Update(current_frame_elapsed_dt);

  if(job.timer.Passed()) {
    job.task.Run(
      current_frame_elapsed_dt
      , /*last_call_elapsed_dt*/ job.timer.GetCurrent()
      , &need_stop_repeating_task);
    job.timer.Reset();
  }

  if(need_stop_repeating_task) {
    // removes the largest element
    task_job_heap_.pop_back();
  }
}

}  // namespace basis
