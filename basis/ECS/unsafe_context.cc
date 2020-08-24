#include "basis/ECS/unsafe_context.hpp" // IWYU pragma: associated

#include <base/memory/singleton.h>
#include <base/no_destructor.h>
#include <base/task/post_task.h>
#include <base/threading/sequence_local_storage_slot.h>
#include <base/threading/sequenced_task_runner_handle.h>
#include <base/memory/ptr_util.h>
#include <base/lazy_instance.h>

#include <memory>

namespace ECS {

UnsafeTypeContext::UnsafeTypeContext()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

UnsafeTypeContext::~UnsafeTypeContext()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

} // namespace ECS
