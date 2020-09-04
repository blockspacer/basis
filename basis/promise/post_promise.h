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
#include "base/message_loop/message_loop.h"
#include "base/task_runner.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/bind.h"

#include <boost/asio.hpp>

#include <boost/beast/core.hpp>

namespace base {

namespace internal {

BASE_EXPORT PassedPromise
PostPromiseInternal(TaskRunner* task_runner,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data,
                    TimeDelta delay);

BASE_EXPORT PassedPromise
PostPromiseInternal(const boost::asio::executor& executor,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data);

BASE_EXPORT PassedPromise
PostPromiseInternal(boost::asio::io_context& context,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data);

// wraps `task.Execute` into `base::OnceClosure`
// used to execute task wrapped in promise
base::OnceClosure ClosureExecutePromise(base::WrappedPromise task);

// Equivalent to PostTask(from_here, task) from task_runner.h
bool PostPromiseHelperInternal(TaskRunner* task_runner
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise
  , TimeDelta delay);

bool PostPromiseHelperInternal(const boost::asio::executor& executor
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise);

bool PostPromiseHelperInternal(boost::asio::io_context& context
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise);

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

// Wraps synchronous task into promise
// that will be executed when synchronous task will be done.
// That approach may not work with async tasks
// (async tasks may require ManualPromiseResolver).
// i.e. async task can return immediately and callback
// for it can be called not in proper moment in time
template <typename CallbackT>
auto PostPromise(const Location& from_here
  , TaskRunner* task_runner
  , CallbackT&& task
  , TimeDelta delay = TimeDelta())
{
  DCHECK(task_runner);
  return PostDelayedPromise(
    from_here, task_runner, std::forward<CallbackT>(task), delay);
}

template <typename CallbackT>
auto PostDelayedPromiseOnExecutor(const Location& from_here,
                     const boost::asio::executor& executor,
                     CallbackT task) {
  // Extract properties from |task| callback.
  using CallbackTraits = internal::CallbackTraits<std::decay_t<CallbackT>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;
  return ReturnedPromise(
    internal::PostPromiseInternal(
     executor, from_here,
     internal::PromiseExecutor::Data(
       in_place_type_t<
         internal::PostTaskExecutor<
           typename CallbackTraits::ReturnType>>(),
       internal::ToCallbackBase(std::move(task))))
  );
}

// Wraps synchronous task into promise
// that will be executed when synchronous task will be done.
// That approach may not work with async tasks
// (async tasks may require ManualPromiseResolver).
// i.e. async task can return immediately and callback
// for it can be called not in proper moment in time
/**
 * \example
  DCHECK(ws_sess);
  const boost::asio::executor& executor
    = ws_sess->stream().get_executor();

  return
    somePromise()
  .ThenHere(FROM_HERE,
    base::BindOnce(
      /// \note returns promise,
      /// so we will wait for NESTED promise
      &PostPromiseAsio<
        base::OnceClosure
      >
      , FROM_HERE
      /// \note |doStartSessionAcceptor| callback
      /// must prolong lifetime of |executor|
      , COPIED(executor)
      , std::move(doStartSessionAcceptor)
    ) // BindOnce
  ) // ThenHere
 **/
template <typename CallbackT>
auto PostPromiseOnAsioExecutor(const Location& from_here
  , const boost::asio::executor& executor
  , CallbackT&& task)
{
  return PostDelayedPromiseOnExecutor(
    from_here, executor, std::forward<CallbackT>(task));
}

template <typename CallbackT>
auto PostDelayedPromiseOnContext(const Location& from_here,
                     boost::asio::io_context& context,
                     CallbackT task) {
  // Extract properties from |task| callback.
  using CallbackTraits = internal::CallbackTraits<std::decay_t<CallbackT>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;
  return ReturnedPromise(
    internal::PostPromiseInternal(
     context, from_here,
     internal::PromiseExecutor::Data(
       in_place_type_t<
         internal::PostTaskExecutor<
           typename CallbackTraits::ReturnType>>(),
       internal::ToCallbackBase(std::move(task))))
  );
}

template <typename CallbackT>
auto PostPromiseOnAsioContext(const Location& from_here
  , boost::asio::io_context& context
  , CallbackT&& task)
{
  return PostDelayedPromiseOnContext(
    from_here, context, std::forward<CallbackT>(task));
}

// Wraps synchronous task into promise
// that will be executed when synchronous task will be done.
// That approach may not work with async tasks
// (async tasks may require ManualPromiseResolver).
// i.e. async task can return immediately and callback
// for it can be called not in proper moment in time
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
        base::internal::PromiseExecutor::Data(
          base::in_place_type_t<
              base::internal::PostTaskExecutor<TaskReturnType>>(),
          base::internal::ToCallbackBase(
            std::move(task)
          )));
  return promise;
}

/// \note waits on posted task runner,
/// so pass different `signalling` task runner to prevent deadlocks
template <typename ResolveType>
void waitForPromiseResolve(
  base::Promise<ResolveType, base::NoReject>& promise
  , scoped_refptr<base::SequencedTaskRunner> signal_task_runner
  , const base::TimeDelta& wait_delta = base::TimeDelta::Max())
{
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL
    , base::WaitableEvent::InitialState::NOT_SIGNALED);

  // `wait` and `signal` must be different sequence
  DCHECK(signal_task_runner
    != base::MessageLoop::current()->task_runner());

  promise
  .ThenOn(signal_task_runner
     , FROM_HERE
     , base::BindOnce(&base::WaitableEvent::Signal, base::Unretained(&event)));

  // The SequencedTaskRunner guarantees that
  // |event| will only be signaled after |task| is executed.
  event.TimedWait(wait_delta);
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
