#pragma once

#include "basis/scoped_checks.hpp"

#include <base/timer/timer.h>
#include <base/time/time.h>
#include <base/bind.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/optional.h>
#include <base/files/file_path.h>
#include <base/trace_event/trace_event.h>
#include <base/synchronization/waitable_event.h>
#include <base/observer_list_threadsafe.h>

#include <boost/asio.hpp>

#include <vector>
#include <optional>

namespace basis {

/**
 * Usage (single threaded):
  {
    /// \note will stop periodic timer on scope exit
    basis::PeriodicTaskExecutor periodicAsioExecutor_1(
      base::BindRepeating(
          [
          ](
            boost::asio::io_context& ioc
          ){
            DCHECK(!ioc.stopped());
            /// \note Runs only on one sequence!
            /// In production create multiple threads
            /// to run |boost::asio::io_context|
            ioc.run_one_for(
              std::chrono::milliseconds{15});
          }
          , std::ref(ioc)
      )
    );

    periodicAsioExecutor_1.startPeriodicTimer(
      base::TimeDelta::FromMilliseconds(30));

    run_loop.Run();
  }
 *
 * Usage (sequence-local-context):
 *
  // Create unique type to store in sequence-local-context
  /// \note initialized, used and destroyed
  /// on `periodicAsioTaskRunner_` sequence-local-context
  using PeriodicAsioExecutorType
    = util::StrongAlias<
        class PeriodicAsioExecutorTag
        /// \note will stop periodic timer on scope exit
        , basis::PeriodicTaskExecutor
      >;
  void Example::setupPeriodicAsioExecutor() NO_EXCEPTION
  {
    DCHECK_THREAD_GUARD_SCOPE(periodicAsioTaskRunner_);

    DCHECK_RUN_ON_SEQUENCED_RUNNER(periodicAsioTaskRunner_.get());

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);
    // Can not register same data type twice.
    // Forces users to call `sequenceLocalContext->unset`.
    DCHECK(!sequenceLocalContext->try_ctx<PeriodicAsioExecutorType>(FROM_HERE));
    PeriodicAsioExecutorType& result
      = sequenceLocalContext->set_once<PeriodicAsioExecutorType>(
          FROM_HERE
          , "PeriodicAsioExecutorType" + FROM_HERE.ToString()
          , base::BindRepeating(
              &Example::updateAsioRegistry
              , base::Unretained(this))
        );
    result->startPeriodicTimer(
      base::TimeDelta::FromMilliseconds(100));
  }

  void Example::deletePeriodicAsioExecutor() NO_EXCEPTION
  {
    DCHECK_THREAD_GUARD_SCOPE(periodicAsioTaskRunner_);

    DCHECK_RUN_ON_SEQUENCED_RUNNER(periodicAsioTaskRunner_.get());

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);
    DCHECK(sequenceLocalContext->try_ctx<PeriodicAsioExecutorType>(FROM_HERE));
    sequenceLocalContext->unset<PeriodicAsioExecutorType>(FROM_HERE);
  }
 **/
/// \note will stop periodic timer on scope exit
/// \note Create, destruct and use on same sequence.
/// See usage example with `sequence-local-context` above.
class PeriodicTaskExecutor
{
 public:
  PeriodicTaskExecutor(
    base::RepeatingClosure&& periodic_task);

  ~PeriodicTaskExecutor();

  void
    setTaskRunner(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  void
    startPeriodicTimer(
      // timer update frequency
      const base::TimeDelta& checkPeriod);

  void
    runOnce();

private:
  void
    restart_timer(
      // timer update frequency
      const base::TimeDelta& checkPeriod);

  void
    shutdown();

private:
  SEQUENCE_CHECKER(sequence_checker_);

  base::RepeatingClosure periodic_task_;

  base::RepeatingTimer timer_;

  scoped_refptr<
      base::SequencedTaskRunner
    > task_runner_;

#if DCHECK_IS_ON()
  std::string debug_guid_;
#endif // DCHECK_IS_ON()

  SET_WEAK_POINTERS(PeriodicTaskExecutor);

  DISALLOW_COPY_AND_ASSIGN(PeriodicTaskExecutor);
};

// executes task periodically on |task_runner|
/// \note do not forget to call |startPeriodicTaskExecutorOnSequence|
void setPeriodicTaskExecutorOnSequence(
  const base::Location& from_here
  , scoped_refptr<base::SequencedTaskRunner> task_runner
  , base::RepeatingClosure updateCallback);

void startPeriodicTaskExecutorOnSequence(
  const base::TimeDelta& endTimeDelta);

void unsetPeriodicTaskExecutorOnSequence();

} // namespace basis
