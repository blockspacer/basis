#include "basis/ECS/global_context.hpp" // IWYU pragma: associated

#if 0
#include <base/memory/singleton.h>
#include <base/no_destructor.h>
#include <base/task/post_task.h>
#include <base/threading/sequence_local_storage_slot.h>
#include <base/threading/sequenced_task_runner_handle.h>
#include <base/memory/ptr_util.h>
#include <base/lazy_instance.h>

#include <memory>

namespace ECS {

UnsafeGlobalContext::UnsafeGlobalContext()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

UnsafeGlobalContext::~UnsafeGlobalContext()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

#if DCHECK_IS_ON()
  for(const UnsafeTypeContext::variable_data& data: context_.vars()) {
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
}

UnsafeGlobalContext*
  UnsafeGlobalContext::GetInstance()
{
  /// Singleton itself thread-safe.
  /// The underlying Type must of course be
  /// thread-safe if you want to use it concurrently.
  return base::Singleton<UnsafeGlobalContext>::get();
}

} // namespace ECS
#endif // 0
