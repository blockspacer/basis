#include "basis/task/periodic_validate_until.h" // IWYU pragma: associated

#include <base/task/thread_pool.h>
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"

#include <basic/macros.h>
#include <basic/promise/post_promise.h>

namespace basis {

PeriodicValidateUntil::PeriodicValidateUntil()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PeriodicValidateUntil::VoidPromise PeriodicValidateUntil::runPromise(
  const ::base::Location& from_here
  , ::basis::EndingTimeout&& debugEndingTimeout
  , ::basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod
  , const std::string& errorText
  , ValidationTaskType&& validationTask)
{
  LOG_CALL(DVLOG(99));

  DCHECK_RUN_ON(&sequence_checker_);

  DCHECK(base::ThreadTaskRunnerHandle::Get());
  // wait and signal on different task runners
  timeoutTaskRunner_ =
    base::ThreadPool::CreateSequencedTaskRunner(
      ::base::TaskTraits{
        ::base::TaskPriority::BEST_EFFORT
        , ::base::MayBlock()
        , ::base::TaskShutdownBehavior::BLOCK_SHUTDOWN
      }
    );

  ignore_result(
    ::base::PostPromise(from_here
      , timeoutTaskRunner_.get()
      , ::base::BindOnce(
          // limit execution time
          &basis::setPeriodicTimeoutCheckerOnSequence
          , from_here
          , timeoutTaskRunner_
          , ::std::move(RVALUE_CAST(debugEndingTimeout))
          // refresh period for (debug-only) execution time limiter
          , /*COPIED*/ checkPeriod
          , errorText))
  );

  periodicVerifyRunner_
    = base::ThreadPool::CreateSequencedTaskRunner(
        ::base::TaskTraits{
          ::base::TaskPriority::BEST_EFFORT
          , ::base::MayBlock()
          , ::base::TaskShutdownBehavior::BLOCK_SHUTDOWN
        }
      );

  return ::base::PostPromise(
    from_here
    // Post our work to the strand, to prevent data race
    , periodicVerifyRunner_.get()
    , ::base::BindOnce(
        &PeriodicValidateUntil::promiseValidationDone
        , ::base::Unretained(this)
        , ::std::move(RVALUE_CAST(validationTask))
          // refresh period for periodic validation
        , /*COPIED*/ checkPeriod
      )
      , ::base::IsNestedPromise{true}
  )
  .ThenOn(periodicVerifyRunner_
    , from_here
    , ::base::BindOnce(&basis::unsetPeriodicTaskExecutorOnSequence)
  )
  /// \note promise has shared lifetime,
  /// so we expect it to exist until (at least)
  /// it is resolved using `GetRepeatingResolveCallback`
  // reset check of execution time
  .ThenOn(timeoutTaskRunner_
    , from_here
    , ::base::BindOnce(&basis::unsetPeriodicTimeoutCheckerOnSequence)
  );
}

PeriodicValidateUntil::VoidPromise
  PeriodicValidateUntil::promiseValidationDone(
    ValidationTaskType&& validationTask
    , ::basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod) NO_EXCEPTION
{
  LOG_CALL(DVLOG(99));

  DCHECK(periodicVerifyRunner_);
  DCHECK_RUN_ON_SEQUENCED_RUNNER(periodicVerifyRunner_.get());

  // promise will be resolved when `validationTask.Run()` returns true
  ::base::ManualPromiseResolver<
      void, ::base::NoReject
    > promiseResolver(FROM_HERE);

  // Bind `GetRepeatingResolveCallback` to passed `validation task`
  DCHECK(validationTask);
  ::base::RepeatingClosure wrappedValidationTask
    = ::base::BindRepeating(
        validationTask
        , /*COPIED*/ promiseResolver.GetRepeatingResolveCallback()
    );

  // check periodically until `validationTask.Run()` returns true
  ::basis::setPeriodicTaskExecutorOnSequence(
    FROM_HERE
    , periodicVerifyRunner_
    , /*COPIED*/ wrappedValidationTask);

  ::basis::startPeriodicTaskExecutorOnSequence(
    checkPeriod.value());

  return promiseResolver.promise();
}

} // namespace basis
