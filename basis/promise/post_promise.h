#pragma once

#include <memory>
#include <utility>

#include "basis/promise/helpers.h"
#include "basis/promise/post_task_executor.h"
#include "basis/promise/promise.h"

#include "base/synchronization/waitable_event.h"
#include "base/base_export.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/time/time.h"
#include "base/updateable_sequenced_task_runner.h"
#include "build/build_config.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/bind.h"

namespace base {

namespace internal {

BASE_EXPORT PassedPromise
PostPromiseInternal(TaskRunner* task_runner,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data,
                    TimeDelta delay);

// wraps `task.Execute` into `base::OnceClosure`
// used to execute task wrapped in promise
base::OnceClosure ClosureExecutePromise(base::WrappedPromise task);

// Equivalent to PostTask(from_here, task) from task_runner.h
bool PostPromiseHelperInternal(TaskRunner* task_runner
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise
  , TimeDelta delay);

}  // namespace internal

template <typename CallbackT>
auto PostDelayedPromise(const Location& from_here,
                     TaskRunner* task_runner,
                     CallbackT task,
                     TimeDelta delay) {
  // Extract properties from |task| callback.
  using CallbackTraits = internal::CallbackTraits<std::decay_t<CallbackT>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;
  return ReturnedPromise(
    internal::PostPromiseInternal(
     task_runner, from_here,
     internal::PromiseExecutor::Data(
       in_place_type_t<
         internal::PostTaskExecutor<
           typename CallbackTraits::ReturnType>>(),
       internal::ToCallbackBase(std::move(task))),
     delay)
  );
}

template <typename CallbackT>
auto PostPromise(const Location& from_here
  , TaskRunner* task_runner
  , CallbackT&& task
  , TimeDelta delay = TimeDelta())
{
  return PostDelayedPromise(
    from_here, task_runner, std::forward<CallbackT>(task), delay);
}


// Wraps synchronous task into promise
// that will be executed when synchronous task will be done.
// That approach may not work with async tasks
// (async tasks may require ManualPromiseResolver).
template <template <typename> class CallbackType,
          typename TaskReturnType,
          typename = EnableIfIsBaseCallback<CallbackType>>
scoped_refptr<base::internal::AbstractPromise>
  promisifySynchronousTask(
    const Location& from_here,
    CallbackType<TaskReturnType()> task)
{
  // Initial PostTask is inlined which results in smaller code.
  using CallbackTraits =
      internal::CallbackTraits<CallbackType<TaskReturnType()>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;

  scoped_refptr<base::internal::AbstractPromise> promise =
    base::internal::AbstractPromise::CreateNoPrerequisitePromise(
        from_here, base::RejectPolicy::kMustCatchRejection,
        base::internal::DependentList::ConstructUnresolved(),
        std::move(base::internal::PromiseExecutor::Data(
          base::in_place_type_t<
              base::internal::PostTaskExecutor<TaskReturnType>>(),
          base::internal::ToCallbackBase(
            std::move(task)
          ))));
  return promise;
}

template <typename ResolveType>
void waitForPromiseResolve(
  const Promise<ResolveType, NoReject>& promise)
{
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL
    , base::WaitableEvent::InitialState::NOT_SIGNALED);

  promise
  .ThenHere(FROM_HERE,
    base::BindOnce(&base::WaitableEvent::Signal, base::Unretained(&event)));

  // The SequencedTaskRunner guarantees that
  // |event| will only be signaled after |task| is executed.
  event.Wait();
}

template <template <typename> class CallbackType,
          typename TaskReturnType,
          typename ReplyArgType,
          typename = EnableIfIsBaseCallback<CallbackType>>
bool PostTaskAndReplyWithPromise(TaskRunner* task_runner,
                                 const Location& from_here,
                                 CallbackType<TaskReturnType()> task,
                                 CallbackType<void(ReplyArgType)> reply) {
  // Initial PostTask is inlined which results in smaller code.
  using CallbackTraits =
      internal::CallbackTraits<CallbackType<TaskReturnType()>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;
  return ReturnedPromise(
             internal::PostPromiseInternal(
                 task_runner, from_here,
                 internal::PromiseExecutor::Data(
                     in_place_type_t<
                         internal::PostTaskExecutor<TaskReturnType>>(),
                     internal::ToCallbackBase(std::move(task))),
                 TimeDelta()))
      .ThenHere(from_here, std::move(reply));
}

}  // namespace base
