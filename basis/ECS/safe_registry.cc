#include "basis/ECS/safe_registry.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/files/file.h>
#include <base/files/file_util.h>
#include <base/files/file_path.h>
#include <base/rvalue_cast.h>
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

namespace ECS {

SafeRegistry::SafeRegistry()
  : ALLOW_THIS_IN_INITIALIZER_LIST(
      weak_ptr_factory_(COPIED(this)))
  , ALLOW_THIS_IN_INITIALIZER_LIST(
      weak_this_(
        weak_ptr_factory_.GetWeakPtr()))
  , taskRunner_{
      ::base::ThreadPool::GetInstance()->
        CreateSequencedTaskRunnerWithTraits(
          ::base::TaskTraits{
            ::base::TaskPriority::BEST_EFFORT
            , ::base::MayBlock()
            , ::base::TaskShutdownBehavior::BLOCK_SHUTDOWN
          }
        )}
  , registry_()
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

SafeRegistry::~SafeRegistry()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

} // namespace ECS
