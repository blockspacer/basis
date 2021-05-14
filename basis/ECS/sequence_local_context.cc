#include "basis/ECS/sequence_local_context.h" // IWYU pragma: associated

#include <base/memory/singleton.h>
#include <base/no_destructor.h>
#include <base/task/post_task.h>
#include <base/threading/sequence_local_storage_slot.h>
#include <base/threading/sequenced_task_runner_handle.h>
#include <base/memory/ptr_util.h>
#include <base/lazy_instance.h>

#include <memory>

namespace ECS {


// Keep the global object in a TLS slot so it is impossible to
// incorrectly from the wrong thread.
static base::LazyInstance<
    base::ThreadLocalPointer<SequenceLocalContext>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

#if 0
namespace {
base::LazyInstance<
    ::base::SequenceLocalStorageSlot<
      scoped_refptr<SequenceLocalContext>
    >
>::Leaky
  g_sls_current_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
TLSSequenceContextStore* TLSSequenceContextStore::current() {
  return lazy_tls.Pointer()->Get();
}

TLSSequenceContextStore::TLSSequenceContextStore() {
  lazy_tls.Pointer()->Set(this);
}

TLSSequenceContextStore::~TLSSequenceContextStore() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  lazy_tls.Pointer()->Set(NULL);
}

void TLSSequenceContextStore::Set(const scoped_refptr<SequenceLocalContext>& value) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  value_ = value;
}

TLSSequenceContextStore* SequenceLocalContext::GetTLSSequenceContextStore()
{
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!tls_store_.get())
    tls_store_.reset(new TLSSequenceContextStore);
  return tls_store_.get();
}
#endif

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

SequenceLocalContext*
  SequenceLocalContext::getLocalInstance(
    const ::base::Location& from_here
    , scoped_refptr<::base::SequencedTaskRunner> task_runner)
{
  DCHECK(base::SequencedTaskRunnerHandle::IsSet())
    << "SequenceLocalStorageSlot cannot be used because no "
       "SequencedTaskRunnerHandle was stored.";

  /**
   * @brief saves from errors like  below
   * (note that `getLocalInstance` called NOT from `timeout_task_runner`):
      .ThenOn(timeout_task_runner
        , FROM_HERE
        , ::base::BindOnce(
            &ECS::SequenceLocalContext::unset<PeriodicCheckUntilTime>
            , ECS::SequenceLocalContext::getLocalInstance(
                FROM_HERE, timeout_task_runner)
            , FROM_HERE
          )
      )
   */
  DCHECK(task_runner
    && task_runner->RunsTasksInCurrentSequence());

  if(!lazy_tls.Pointer()->Get())
  {
    DVLOG(9)
      << "created new SequenceLocalContext from "
      << from_here.ToString();

    lazy_tls.Pointer()->Set(
      new SequenceLocalContext());
  } else {
    DVLOG(9)
      << "re-using existing SequenceLocalContext from "
      << from_here.ToString();
  }

  // `Get` Sets and returns a default-constructed value
  // if no value was previously set.
  SequenceLocalContext* ctx
    = lazy_tls.Pointer()->Get();

  DCHECK(ctx)
    << "SequenceLocalStorageSlot cannot be used because no "
       "SequenceLocalContext was stored in TLS.";

  return ctx;
}

} // namespace ECS
