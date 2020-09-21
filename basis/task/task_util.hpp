#pragma once

#include <base/logging.h>
#include <base/location.h>
#include <base/callback_forward.h>
#include <base/sequenced_task_runner.h>

namespace basis {

bool RunsTasksInAnySequenceOf(
  const std::vector<scoped_refptr<base::SequencedTaskRunner>>& task_runners
  , bool dcheck_not_empty = true);

// Posts |task| to |task_runner| and blocks until it is executed.
void PostTaskAndWait(const base::Location& from_here
  , base::SequencedTaskRunner* task_runner
  , base::OnceClosure task);

// Redirects task to task runner.
//
// USAGE
//
//   base::OnceClosure task
//     = base::internal::bindToTaskRunner(
//         FROM_HERE,
//         base::BindOnce(
//             &ExampleServer::doQuit
//             , base::Unretained(this)),
//         base::MessageLoop::current()->task_runner())
BASE_EXPORT
MUST_USE_RETURN_VALUE
base::OnceClosure bindToTaskRunner(
  const base::Location& from_here,
  base::OnceClosure&& task,
  scoped_refptr<base::SequencedTaskRunner> task_runner,
  base::TimeDelta delay = base::TimeDelta()) NO_EXCEPTION;

} // namespace basis
