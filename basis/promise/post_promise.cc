#include "basis/promise/post_promise.h"  // IWYU pragma: associated

#include "basis/task_run/task_run_util.hpp"

#include <utility>

#include "base/logging.h"
#include "base/task/scoped_set_task_priority_for_current_thread.h"
#include "base/task/task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"

namespace base {

namespace internal {

base::OnceClosure ClosureExecutePromise(base::WrappedPromise task) {
  return base::BindOnce(
    [](base::WrappedPromise task) {
      task.Execute();
    }
    , std::move(task));
}

bool PostPromiseHelperInternal(TaskRunner* task_runner
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise
  , TimeDelta delay)
{
  return task_runner->PostDelayedTask(from_here,
    ClosureExecutePromise(WrappedPromise(promise)), delay);
}

// To keep PostTask related binary size down we want to do this out of line.
PassedPromise PostPromiseInternal(
    TaskRunner* task_runner,
    const Location& from_here,
    internal::PromiseExecutor::Data&& executor_data,
    TimeDelta delay)
{
  scoped_refptr<AbstractPromise> promise =
    AbstractPromise::CreateNoPrerequisitePromise(
      from_here, RejectPolicy::kMustCatchRejection,
      internal::DependentList::ConstructUnresolved(),
      std::move(executor_data));

  if (!internal::PostPromiseHelperInternal(
        task_runner, from_here, promise, delay))
  {
    return PassedPromise();
  }

  return PassedPromise(std::move(promise));
}

}  // namespace internal

}  // namespace base
