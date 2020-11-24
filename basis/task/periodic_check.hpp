#pragma once


#include "basis/scoped_checks.hpp"
#include "basis/strong_types/strong_alias.hpp"
#include "basis/promise/promise.h"
#include "basis/promise/helpers.h"
#include "basis/promise/post_task_executor.h"

#include <base/timer/timer.h>
#include <base/time/time.h>
#include <base/bind.h>
#include <base/optional.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/optional.h>
#include <base/files/file_path.h>
#include <base/trace_event/trace_event.h>
#include <base/synchronization/waitable_event.h>
#include <base/observer_list_threadsafe.h>
#include <base/thread_annotations.h>

#include <vector>
#include <optional>

namespace basis {

class CheckUntilObserver {
 public:
  CheckUntilObserver();

  virtual
    ~CheckUntilObserver();

  virtual void
    OnCheckUntil() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CheckUntilObserver);
};

// Runs |CheckNotifyTask| periodically until |CheckShutdownTask|.
// Will |NotifyObservers| when |CheckNotifyTask| returns true.
// Will call |shutdown| when |CheckShutdownTask| returns true.
// Stops all periodic checks on destruction.
class PeriodicCheckUntil
{
 public:
   using CheckNotifyTask
    =  ::base::RepeatingCallback<bool()>;

   using CheckShutdownTask
    =  ::base::RepeatingCallback<bool()>;

  STRONGLY_TYPED(base::TimeDelta, CheckPeriod);

 public:
  PeriodicCheckUntil(
    scoped_refptr<::base::SequencedTaskRunner> task_runner
    , CheckNotifyTask&& checkNotifyTask
    , CheckShutdownTask&& checkShutdownTask);

  // calls startPeriodicTimer
  PeriodicCheckUntil(
    scoped_refptr<::base::SequencedTaskRunner> task_runner
    , CheckNotifyTask&& checkNotifyTask
    , CheckShutdownTask&& checkShutdownTask
    // timer update frequency
    , const CheckPeriod& checkPeriod);

  ~PeriodicCheckUntil();

  // Add a non owning pointer
  void
    AddObserver(
      CheckUntilObserver* observer);

  // Does nothing if the |observer| is
  // not in the list of known observers.
  void
    RemoveObserver(
      CheckUntilObserver* observer);

  // Notify |Observer|s
  void
    NotifyObservers();

  void
    startPeriodicTimer(
      // timer update frequency
      const CheckPeriod& checkPeriod);

  void
    runOnce();

private:
  void
    restart_timer(
      // timer update frequency
      const CheckPeriod& checkPeriod);

  void
    shutdown();

private:
  SEQUENCE_CHECKER(sequence_checker_);

  friend class CheckUntilObserver;

  /// \note created and destroyed on |sequence_checker_|,
  /// but used on |task_runner_|
  ::base::RepeatingTimer timer_;

  /// \note ObserverListThreadSafe may be ued from multiple threads
  const scoped_refptr<
      ::base::ObserverListThreadSafe<CheckUntilObserver>
    > observers_;

  scoped_refptr<
      ::base::SequencedTaskRunner
    > task_runner_;

  CheckNotifyTask checkNotifyTask_;

  CheckShutdownTask checkShutdownTask_;

  SET_WEAK_POINTERS(PeriodicCheckUntil);

  DISALLOW_COPY_AND_ASSIGN(PeriodicCheckUntil);
};

/// \todo move |EndingTimeout|, |PeriodicCheckUntilTime| to separate files
#include <base/time/time.h>

class EndingTimeout {
 public:
  explicit EndingTimeout(
    const ::base::Time& endTime);

  // will calculate |endTime| as (base::Time::Now() + endTimeDelta)
  explicit EndingTimeout(
    const ::base::TimeDelta& endTimeDelta);

  EndingTimeout(
    EndingTimeout&& other)
  {
    endTime_ = std::move(other.endTime_);
    /// \note do not move |sequence_checker_|
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  EndingTimeout& operator=(
    EndingTimeout&& other)
  {
    endTime_ = std::move(other.endTime_);
    /// \note do not move |sequence_checker_|
    DETACH_FROM_SEQUENCE(sequence_checker_);
    return *this;
  }

  ~EndingTimeout();

  ::base::Time endTime() const
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return endTime_;
  }

private:
  SEQUENCE_CHECKER(sequence_checker_);

  ::base::Time endTime_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EndingTimeout);
};

// Runs |expiredCallback| when |endTime| reached.
// Stops all periodic checks on destruction.
/// \note This can be useful in diagnosing
/// deadlocks, stalls and memory leaks
/// without logging too agressively.
/**
 * \example
   DCHECK(base::ThreadPool::GetInstance());
   // wait and signal on different task runners
   scoped_refptr<::base::SequencedTaskRunner> timeout_task_runner =
     ::base::ThreadPool::GetInstance()->
     CreateSequencedTaskRunnerWithTraits(
       ::base::TaskTraits{
         ::base::TaskPriority::BEST_EFFORT
         , ::base::MayBlock()
         , ::base::TaskShutdownBehavior::BLOCK_SHUTDOWN
       }
     );
   appLoopRunner.promiseFirstRun()
   .ThenOn(timeout_task_runner
     , FROM_HERE
     , ::base::BindOnce(
       [
       ](
         std::unique_ptr<PeriodicCheckUntilTime>& timeoutForInit
       ){
         DCHECK(!base::RunLoop::IsRunningOnCurrentThread());
         DCHECK(base::SequencedTaskRunnerHandle::Get());

         EndingTimeout endingTimeout(
           // end time = now() + delta
           ::base::TimeDelta::FromSeconds(5));

         DCHECK(!timeoutForInit);
         timeoutForInit = std::make_unique<PeriodicCheckUntilTime>(
           // same as timeout_task_runner
           ::base::SequencedTaskRunnerHandle::Get()
           , ::base::BindRepeating([
             ](
             ){
               LOG(WARNING)
                 << "application initialization hanged";
               /// \note will continue execution in production
               DCHECK(false)
                 << "application initialization hanged";
             })
           , endingTimeout
           // timer update frequency
           , PeriodicCheckUntil::CheckPeriod{
               ::base::TimeDelta::FromMilliseconds(500)}
         );
       }
       , std::ref(timeoutForInit_) /// \note must manage lifetime
   ))
   .ThenOn(base::MessageLoop::current()->task_runner()
     , FROM_HERE
     , ::base::BindOnce(
         &ServerEnvironment::DoTasks
         , ::base::Unretained(this)
   ))
   // reset |timeoutForInit_|
   .ThenOn(timeout_task_runner
     , FROM_HERE
     , ::base::BindOnce(
         &std::unique_ptr<PeriodicCheckUntilTime>::reset
         , ::base::Unretained(&timeoutForInit_)
         , nullptr // reset unique_ptr to nullptr
       )
   );
 **/
class PeriodicCheckUntilTime
{
 public:
  using OptionalCheckPeriod
    = ::base::Optional<PeriodicCheckUntil::CheckPeriod>;

  PeriodicCheckUntilTime(
    scoped_refptr<::base::SequencedTaskRunner> task_runner
    , ::base::RepeatingClosure&& expiredCallback
    , const EndingTimeout& endTime
    // timer update frequency
    // calls startPeriodicTimer if set
    , const OptionalCheckPeriod& optionalCheckPeriod);

  ~PeriodicCheckUntilTime();

  void
    startPeriodicTimer(
      // timer update frequency
      const PeriodicCheckUntil::CheckPeriod& checkPeriod);

private:
  SEQUENCE_CHECKER(sequence_checker_);

  PeriodicCheckUntil periodicCheckUntil_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PeriodicCheckUntilTime);
};

// Can be used to limit execution time.
// Will create |PeriodicCheckUntilTime|
// and store it into |SequenceLocalContext|.
/// \note Do not forget to call |unsetPeriodicTimeoutCheckerOnSequence|
/// \note In release builds will print warning on timeout.
/// In debug builds will print warning AND `DCHECK` on timeout.
/**
 * \usage
  DCHECK(base::ThreadPool::GetInstance());
  // wait and signal on different task runners
  scoped_refptr<::base::SequencedTaskRunner> timeout_task_runner =
    ::base::ThreadPool::GetInstance()->
    CreateSequencedTaskRunnerWithTraits(
      ::base::TaskTraits{
        ::base::TaskPriority::BEST_EFFORT
        , ::base::MayBlock()
        , ::base::TaskShutdownBehavior::BLOCK_SHUTDOWN
      }
    );
  somePromise()
  .ThenOn(timeout_task_runner
    , FROM_HERE
    , ::base::BindOnce(
        // limit execution time
        &setPeriodicTimeoutCheckerOnSequence
        , FROM_HERE
        , timeout_task_runner
        , EndingTimeout{
            ::base::TimeDelta::FromSeconds(5)}
        , PeriodicCheckUntil::CheckPeriod{
            ::base::TimeDelta::FromMilliseconds(500)}
        , "application initialization hanged"
  ))
  .ThenOn(base::MessageLoop::current()->task_runner()
    , FROM_HERE
    , ::base::BindOnce(
        &doSomething
  ))
  // reset check of execution time
  .ThenOn(timeout_task_runner
    , FROM_HERE
    , ::base::BindOnce(&unsetPeriodicTimeoutCheckerOnSequence)
  )
  ;
 */
void setPeriodicTimeoutCheckerOnSequence(
  const ::base::Location& from_here
  , scoped_refptr<::base::SequencedTaskRunner> task_runner
  , const EndingTimeout& endingTimeout
  , const PeriodicCheckUntil::CheckPeriod& checkPeriod
  , const std::string& errorText);

// Will unset |PeriodicCheckUntilTime| on |SequenceLocalContext|.
void unsetPeriodicTimeoutCheckerOnSequence();

} // namespace basis
