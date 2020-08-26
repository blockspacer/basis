#include "basis/ECS/asio_registry.hpp" // IWYU pragma: associated

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

namespace ECS {

AsioRegistry::AsioRegistry(
  util::UnownedPtr<IoContext>&& ioc)
  : weak_this_factory_(this)
  , asioRegistryStrand_(ioc.Get()->get_executor())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AsioRegistry::~AsioRegistry()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

Registry &AsioRegistry::ref_registry_unsafe(const base::Location& from_here) noexcept
{
  ignore_result(from_here);
  return registry_;
}

Registry &AsioRegistry::ref_registry(const base::Location& from_here) noexcept
{
  DCHECK(asioRegistryStrand_.running_in_this_thread())
    << from_here.ToString();
  return registry_;
}

} // namespace ECS
