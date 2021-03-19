#include "basis/task/periodic_check.hpp" // IWYU pragma: associated

#include "basis/ECS/sequence_local_context.hpp"

#include <base/logging.h>
#include <base/files/file.h>
#include <base/files/file_util.h>
#include <base/files/file_path.h>
#include <base/trace_event/trace_event.h>
#include <base/threading/scoped_blocking_call.h>
#include <base/threading/thread_restrictions.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/timer/timer.h>
#include <base/path_service.h>
#include <base/sequenced_task_runner.h>
#include <base/task/post_task.h>
#include <base/task/task_traits.h>
#include <base/trace_event/trace_event.h>
#include <base/compiler_specific.h>
#include <basic/rvalue_cast.h>

#include <basis/promise/post_promise.h>
#include <basis/tracing/trace_event_util.hpp>
#include <basis/application/application_configuration.hpp>

namespace basis {

CheckUntilObserver::CheckUntilObserver() = default;

CheckUntilObserver::~CheckUntilObserver() = default;

PeriodicCheckUntil::PeriodicCheckUntil(
  scoped_refptr<::base::SequencedTaskRunner> task_runner
  , CheckNotifyTask&& checkNotifyTask
  , CheckShutdownTask&& checkShutdownTask)
  : task_runner_(task_runner)
  , checkNotifyTask_(RVALUE_CAST(checkNotifyTask))
  , checkShutdownTask_(RVALUE_CAST(checkShutdownTask))
  , ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this))
  , ALLOW_THIS_IN_INITIALIZER_LIST(
      weak_this_(weak_ptr_factory_.GetWeakPtr()))
  , observers_(new ::base::ObserverListThreadSafe<CheckUntilObserver>())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(task_runner_);
}

PeriodicCheckUntil::PeriodicCheckUntil(
  scoped_refptr<::base::SequencedTaskRunner> task_runner
  , CheckNotifyTask&& checkNotifyTask
  , CheckShutdownTask&& checkShutdownTask
  , const CheckPeriod& checkPeriod)
  : PeriodicCheckUntil(task_runner
      , RVALUE_CAST(checkNotifyTask)
      , RVALUE_CAST(checkShutdownTask))
{
  startPeriodicTimer(checkPeriod);
}

PeriodicCheckUntil::~PeriodicCheckUntil()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  shutdown();

  // All observers must be gone now:
  // unregister observers before, in their own Shutdown(), and all others
  // should have done it now when they got the shutdown notification.
  // Note: "might_have_observers" sounds like it might be inaccurate, but it can
  // only return false positives while an iteration over the ObserverList is
  // ongoing.
  //DCHECK(!observers_->might_have_observers());
#if DCHECK_IS_ON()
  DCHECK(observers_);
  observers_->AssertEmpty();
#endif // DCHECK_IS_ON()
}

void
  PeriodicCheckUntil::NotifyObservers()
{
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  DCHECK(observers_);
  observers_->Notify(FROM_HERE
    , &CheckUntilObserver::OnCheckUntil);
}

void
  PeriodicCheckUntil::AddObserver(
    CheckUntilObserver* observer)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(observer);
  DCHECK(observers_);
  observers_->AddObserver(observer);
}

void
  PeriodicCheckUntil::RemoveObserver(
    CheckUntilObserver* observer)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(observer);
  DCHECK(observers_);
  observers_->RemoveObserver(observer);
}

void
  PeriodicCheckUntil::startPeriodicTimer(
    const CheckPeriod& checkPeriod)
{
  LOG_CALL(DVLOG(99));

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(!timer_.IsRunning());

  const bool postTaskOk
    = task_runner_->PostTask(FROM_HERE
      , ::base::Bind(&PeriodicCheckUntil::restart_timer
                   , weak_this_
                   , /*copied*/checkPeriod)
    );
  DCHECK(postTaskOk);
}

void
  PeriodicCheckUntil::restart_timer(
    const CheckPeriod& checkPeriod)
{
  LOG_CALL(DVLOG(99));

  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // It's safe to destroy or restart Timer on another sequence after Stop().
  timer_.Stop();
  timer_.Reset(); // abandon scheduled task
  timer_.Start(FROM_HERE
    , checkPeriod.value()
    , this
    , &PeriodicCheckUntil::runOnce
  );
  DCHECK_EQ(timer_.GetCurrentDelay(), checkPeriod.value());
}

void
  PeriodicCheckUntil::runOnce()
{
  TRACE_EVENT0("headless"
    , "PeriodicCheckUntil_runOnce");

  DVLOG(9999)
    << "(PeriodicCheckUntil) runOnce...";

  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  DCHECK(checkNotifyTask_);
  if(checkNotifyTask_.Run())
  {
    NotifyObservers();

    DCHECK(checkShutdownTask_);
    if(checkShutdownTask_.Run())
    {
      shutdown();
    }
  }

  DVLOG(9999)
    << "(PeriodicCheckUntil) finished runOnce...";
}

void
  PeriodicCheckUntil::shutdown()
{
  LOG_CALL(DVLOG(99));

  if(timer_.IsRunning())
  {
    timer_.Stop();
  }
}

PeriodicCheckUntilTime::PeriodicCheckUntilTime(
  scoped_refptr<::base::SequencedTaskRunner> task_runner
  , ::base::RepeatingClosure&& expiredCallback
  , const EndingTimeout& endTime
  , const ::base::Optional<PeriodicCheckUntil::CheckPeriod>& optionalCheckPeriod)
  : periodicCheckUntil_(
      task_runner
      // Will |NotifyObservers| when |CheckNotifyTask| returns true.
      , ::base::BindRepeating([
        ](
          const ::base::Time endTime
          , ::base::RepeatingClosure expiredCallback
        )
          -> bool
        {
          const bool isExpired
            = ::base::Time::Now() > endTime;
          if(isExpired)
          {
            DCHECK(expiredCallback);
            expiredCallback.Run();

            // will notify observers
            return true;
          }
          return false;
        }
        , endTime.endTime() /// \note copied
        , expiredCallback /// \note copied
        )
      // Will call |shutdown| when |CheckShutdownTask| returns true.
      , ::base::BindRepeating([
        ](
          const ::base::Time endTime
        )
          -> bool
        {
          const bool isExpired
            = ::base::Time::Now() > endTime;
          if(isExpired)
          {
            // will stop timer
            return true;
          }
          return false;
        }
        , endTime.endTime() /// \note copied
        )
    ) // periodicCheckUntil_
{
  DETACH_FROM_SEQUENCE(sequence_checker_);

  if(optionalCheckPeriod.has_value())
  {
    const PeriodicCheckUntil::CheckPeriod& checkPeriod
      = optionalCheckPeriod.value();
    periodicCheckUntil_.startPeriodicTimer(checkPeriod);
  }
}

PeriodicCheckUntilTime::~PeriodicCheckUntilTime()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void PeriodicCheckUntilTime::startPeriodicTimer(
  const PeriodicCheckUntil::CheckPeriod &checkPeriod)
{
  periodicCheckUntil_.startPeriodicTimer(checkPeriod);
}

EndingTimeout::EndingTimeout(
  const ::base::Time& endTime)
  : endTime_(endTime)
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

basis::EndingTimeout::EndingTimeout(
  const ::base::TimeDelta& endTimeDelta)
  : EndingTimeout(base::Time::Now() + endTimeDelta)
{}

basis::EndingTimeout::~EndingTimeout()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void setPeriodicTimeoutCheckerOnSequence(
  const ::base::Location& from_here
  , scoped_refptr<::base::SequencedTaskRunner> task_runner
  , const EndingTimeout& endingTimeout
  , const PeriodicCheckUntil::CheckPeriod& checkPeriod
  , const std::string& errorText)
{
  LOG_CALL(DVLOG(99));

  DCHECK(task_runner);

  ::base::RepeatingClosure errorCallback
    = ::base::BindRepeating([
      ](
        const std::string& errorText
      ){
        LOG(WARNING)
          << errorText;
        /// \note will continue execution in production
        DCHECK(false)
          << errorText;
      }
      , errorText /// \note copied
      );

  ::base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
    = ECS::SequenceLocalContext::getSequenceLocalInstance(
        from_here, task_runner);

  DCHECK(sequenceLocalContext);
  ignore_result(sequenceLocalContext->set_once<PeriodicCheckUntilTime>(
        from_here
        , "Timeout.PeriodicCheckUntilTime." + from_here.ToString()
        , task_runner
        , RVALUE_CAST(errorCallback)
        , endingTimeout
        // timer update frequency
        , checkPeriod
      ));
}

void unsetPeriodicTimeoutCheckerOnSequence()
{
  LOG_CALL(DVLOG(99));

  ::base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
    = ECS::SequenceLocalContext::getSequenceLocalInstance(
        FROM_HERE, ::base::SequencedTaskRunnerHandle::Get());

  DCHECK(sequenceLocalContext);
  sequenceLocalContext->unset<PeriodicCheckUntilTime>(FROM_HERE);
}

} // namespace basis
