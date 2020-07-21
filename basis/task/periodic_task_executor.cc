#include "basis/task/periodic_task_executor.hpp" // IWYU pragma: associated

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

#include <basis/promise/promise.h>
#include <basis/promise/helpers.h>
#include <basis/promise/post_task_executor.h>
#include <basis/trace_event_util.hpp>
#include <basis/application/application_configuration.hpp>

namespace basis {

PeriodicTaskExecutor::PeriodicTaskExecutor(
  scoped_refptr<base::SequencedTaskRunner> task_runner
  , base::RepeatingClosure&& periodic_task)
  : task_runner_(task_runner)
  , periodic_task_(std::move(periodic_task))
  , ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this))
  , ALLOW_THIS_IN_INITIALIZER_LIST(
      weak_this_(weak_ptr_factory_.GetWeakPtr()))
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(task_runner_);
}

PeriodicTaskExecutor::~PeriodicTaskExecutor()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  shutdown();
}

void
  PeriodicTaskExecutor::startPeriodicTimer(
    const base::TimeDelta& checkPeriod)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  LOG(INFO) << "(PeriodicTaskExecutor) startup";

  DCHECK(!timer_.IsRunning());

  const bool postTaskOk
    = task_runner_->PostTask(FROM_HERE
      , base::Bind(&PeriodicTaskExecutor::restart_timer
                   , weak_this_
                   , /*copied*/checkPeriod)
    );
  DCHECK(postTaskOk);
}

void
  PeriodicTaskExecutor::restart_timer(
    const base::TimeDelta& checkPeriod)
{
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  LOG(INFO) << "(PeriodicTaskExecutor) restart_timer";

  // It's safe to destroy or restart Timer on another sequence after Stop().
  timer_.Stop();
  timer_.Reset(); // abandon scheduled task
  timer_.Start(FROM_HERE
    , checkPeriod
    , this
    , &PeriodicTaskExecutor::runOnce
  );
  DCHECK_EQ(timer_.GetCurrentDelay(), checkPeriod);
}

void
  PeriodicTaskExecutor::runOnce()
{
  TRACE_EVENT0("headless"
    , "PeriodicTaskExecutor_runOnce");

  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  periodic_task_.Run();
}

void
  PeriodicTaskExecutor::shutdown()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  LOG(INFO) << "(PeriodicTaskExecutor) shutdown";
  timer_.Stop();
}

} // namespace basis
