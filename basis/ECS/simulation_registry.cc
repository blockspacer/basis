#include "basis/ECS/simulation_registry.hpp" // IWYU pragma: associated

#if 0
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
//#include <base/strings/string_number_conversions.h>
//#include <base/strings/string_util.h>
//#include <base/strings/utf_string_conversions.h>
#include <base/task/post_task.h>
#include <base/task/task_traits.h>
#include <base/trace_event/trace_event.h>

namespace ECS {

SimulationRegistry::SimulationRegistry()
  : weak_this_factory_(this)
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

void SimulationRegistry::set_task_runner(
  scoped_refptr<base::SequencedTaskRunner> task_runner) noexcept
{
  DCHECK(!task_runner_);
  task_runner_ = task_runner;
  DCHECK(task_runner_);
}

SimulationRegistry::~SimulationRegistry()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

Registry &SimulationRegistry::registry_unsafe(const base::Location& from_here) noexcept
{
  ignore_result(from_here);
  return registry_;
}

Registry &SimulationRegistry::registry() noexcept
{
  DCHECK(task_runner_)
    << FROM_HERE.ToString();
  DCHECK(task_runner_->RunsTasksInCurrentSequence())
    << FROM_HERE.ToString();
  return registry_;
}

} // namespace ECS
#endif // 0
