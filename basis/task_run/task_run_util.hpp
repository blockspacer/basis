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

} // namespace basis
