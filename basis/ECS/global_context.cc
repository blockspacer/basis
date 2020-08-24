#include "basis/ECS/global_context.hpp" // IWYU pragma: associated

#include <base/memory/singleton.h>
#include <base/no_destructor.h>
#include <base/task/post_task.h>
#include <base/threading/sequence_local_storage_slot.h>
#include <base/threading/sequenced_task_runner_handle.h>
#include <base/memory/ptr_util.h>
#include <base/lazy_instance.h>

#include <memory>

namespace ECS {

GlobalContext::GlobalContext()
{
  DETACH_FROM_THREAD(main_thread_checker_);
}

GlobalContext::~GlobalContext()
{
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

#if DCHECK_IS_ON()
  for(const UnsafeTypeContext::variable_data& data: context_.ref_vars()) {
    LOG(ERROR)
      << "You must manually call `unset` for: "
      << data.debug_name;
  }
#endif // DCHECK_IS_ON()

  /// \note allows to assume that all resources
  /// are manually freed in proper order
  DCHECK(context_.empty())
    << "You must manually call `unset` before destruction."
    << "Remaining elements count:"
    << context_.size();

  DCHECK(!locked_.load())
    << "global context must be unlocked during destruction";
}

GlobalContext*
  GlobalContext::GetInstance()
{
  /// Singleton itself thread-safe.
  /// The underlying Type must of course be
  /// thread-safe if you want to use it concurrently.
  return base::Singleton<GlobalContext>::get();
}

void ECS::GlobalContext::lockModification()
{
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  DCHECK(!locked_.load())
    << "modification of global context already locked";
  locked_ = true;

  DVLOG(9)
    << "locked GlobalContext";
}

void GlobalContext::unlockModification()
{
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  DCHECK(locked_.load())
    << "modification of global context already unlocked";
  locked_ = false;

  DVLOG(9)
      << "unlocked GlobalContext";
}

} // namespace ECS
