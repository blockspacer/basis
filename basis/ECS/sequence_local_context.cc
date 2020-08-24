#include "basis/ECS/sequence_local_context.hpp" // IWYU pragma: associated

#include <base/memory/singleton.h>
#include <base/no_destructor.h>
#include <base/task/post_task.h>
#include <base/threading/sequence_local_storage_slot.h>
#include <base/threading/sequenced_task_runner_handle.h>
#include <base/memory/ptr_util.h>
#include <base/lazy_instance.h>

#include <memory>

namespace ECS {

namespace {
base::LazyInstance<
    base::SequenceLocalStorageSlot<
      scoped_refptr<SequenceLocalContext>
    >
>::Leaky
  g_sls_current_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;
}  // namespace

SequenceLocalContext::SequenceLocalContext()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

SequenceLocalContext::~SequenceLocalContext()
{
  /// \note That check may fail and we do not need it.
  /// Usually sequence-bound and ref-counted |SequenceLocalContext|
  /// will be destroyed when parent sequence is destroying.
  /// --> DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

base::WeakPtr<SequenceLocalContext>
  SequenceLocalContext::getSequenceLocalInstance(
    const base::Location& from_here
    , scoped_refptr<base::SequencedTaskRunner> task_runner)
{
  DCHECK(base::SequencedTaskRunnerHandle::IsSet())
    << "SequenceLocalStorageSlot cannot be used because no "
       "SequencedTaskRunnerHandle was stored.";

  /**
   * @brief saves from errors like  below
   * (note that `getSequenceLocalInstance` called NOT from `timeout_task_runner`):
      .ThenOn(timeout_task_runner
        , FROM_HERE
        , base::BindOnce(
            &ECS::SequenceLocalContext::unset<PeriodicCheckUntilTime>
            , ECS::SequenceLocalContext::getSequenceLocalInstance(
                FROM_HERE, timeout_task_runner)
            , FROM_HERE
          )
      )
   */
  DCHECK(task_runner
    && task_runner->RunsTasksInCurrentSequence());

  if(!g_sls_current_sequence_local_storage.Get().Get())
  {
    DVLOG(9)
      << "created new SequenceLocalContext from "
      << from_here.ToString();

    g_sls_current_sequence_local_storage.Get().Set(
      new SequenceLocalContext());
  } else {
    DVLOG(9)
      << "re-using existing SequenceLocalContext from "
      << from_here.ToString();
  }

  // `Get` Sets and returns a default-constructed value
  // if no value was previously set.
  scoped_refptr<SequenceLocalContext>& ctx
    = g_sls_current_sequence_local_storage.Get().Get();

  DCHECK(ctx)
    << "SequenceLocalStorageSlot cannot be used because no "
       "SequenceLocalContext was stored in TLS.";

  return ctx->weak_ptr_factory_.GetWeakPtr();
}

} // namespace ECS
