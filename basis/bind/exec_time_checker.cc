#include "basis/bind/exec_time_checker.hpp" // IWYU pragma: associated

#include <basis/ECS/sequence_local_context.hpp>
#include <basis/strong_types/strong_alias.hpp>

#include <base/threading/sequenced_task_runner_handle.h>

namespace base {

STRONGLY_TYPED(base::Time, PerSequenceExecTimeCheckerStartTime);

void perSequenceStoreTimeBeforeCallbackExecution()
{
  DCHECK(base::SequencedTaskRunnerHandle::IsSet())
    << "Sequence must be set "
    << FROM_HERE.ToString();

  ::base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
    = ECS::SequenceLocalContext::getSequenceLocalInstance(
        FROM_HERE, ::base::SequencedTaskRunnerHandle::Get());

  DCHECK(sequenceLocalContext);
  ignore_result(sequenceLocalContext->set_once<PerSequenceExecTimeCheckerStartTime>(
        FROM_HERE
        , "Timeout.PerSequenceExecTimeCheckerStartTime." + FROM_HERE.ToString()
        , ::base::in_place
        , ::base::Time::Now()
      ));
}

base::Time perSequenceGetTimeBeforeCallbackExecution()
{
  DCHECK(base::SequencedTaskRunnerHandle::IsSet())
    << "Sequence must be set "
    << FROM_HERE.ToString();

  ::base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
    = ECS::SequenceLocalContext::getSequenceLocalInstance(
        FROM_HERE, ::base::SequencedTaskRunnerHandle::Get());

  DCHECK(sequenceLocalContext);
  PerSequenceExecTimeCheckerStartTime& result
    = sequenceLocalContext->ctx<PerSequenceExecTimeCheckerStartTime>(
        FROM_HERE
      );

  return *result;
}

void perSequenceClearTimeBeforeCallbackExecution()
{
  DCHECK(base::SequencedTaskRunnerHandle::IsSet())
    << "Sequence must be set "
    << FROM_HERE.ToString();

  ::base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
    = ECS::SequenceLocalContext::getSequenceLocalInstance(
        FROM_HERE, ::base::SequencedTaskRunnerHandle::Get());

  DCHECK(sequenceLocalContext);
  sequenceLocalContext->unset<PerSequenceExecTimeCheckerStartTime>(
    FROM_HERE
  );
}

ExecTimeChecker bindExecTimeChecker(
  const ::base::Location& location
  , const ::base::TimeDelta& val)
{
  return ExecTimeChecker{
    location
    , val
  };
}

} // namespace base
