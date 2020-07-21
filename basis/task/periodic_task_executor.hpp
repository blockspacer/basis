#pragma once

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

#include <vector>
#include <optional>

namespace basis {

/**
 * Usage:
  {
    /// \note will stop periodic timer on scope exit
    basis::PeriodicTaskExecutor periodicAsioExecutor_1(
      asio_task_runner
      , base::BindRepeating(
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
 **/
/// \note will stop periodic timer on scope exit
class PeriodicTaskExecutor
{
 public:
  PeriodicTaskExecutor(
    scoped_refptr<base::SequencedTaskRunner> task_runner
    , base::RepeatingClosure&& periodic_task);

  ~PeriodicTaskExecutor();

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

  /// \note created and destroyed on |sequence_checker_|,
  /// but used on |task_runner_|
  base::RepeatingTimer timer_;

  scoped_refptr<
      base::SequencedTaskRunner
    > task_runner_;

  // base::WeakPtr can be used to ensure that any callback bound
  // to an object is canceled when that object is destroyed
  // (guarantees that |this| will not be used-after-free).
  base::WeakPtrFactory<
      PeriodicTaskExecutor
    > weak_ptr_factory_;

  // After constructing |weak_ptr_factory_|
  // we immediately construct a WeakPtr
  // in order to bind the WeakPtr object to its thread.
  // When we need a WeakPtr, we copy construct this,
  // which is safe to do from any
  // thread according to weak_ptr.h (versus calling
  // |weak_ptr_factory_.GetWeakPtr() which is not).
  base::WeakPtr<PeriodicTaskExecutor> weak_this_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PeriodicTaskExecutor);
};

} // namespace basis
