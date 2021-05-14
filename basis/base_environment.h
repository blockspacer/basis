#pragma once

#include <base/at_exit.h>
#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/scoped_refptr.h>
#include "base/run_loop.h"
#include <base/sequence_checker.h>
#include <base/macros.h>
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/message_loop/message_pump_type.h"
#include "base/threading/hang_watcher.h"

#include <basic/macros.h>
#include <basic/log/scoped_log_run_time.h>

#include <string>
#include <vector>

namespace base {
class FeatureList;
class SingleThreadTaskRunner;
} // namespace base

namespace entt { class dispatcher; }

namespace i18n { class I18n; }

namespace basis {

/// \note must store data related to base and basis libs
/// inits basic requirements, like thread pool, logging, etc.
class ScopedBaseEnvironment {
public:
  ScopedBaseEnvironment();

  ~ScopedBaseEnvironment();

  // init with provided settings
  MUST_USE_RETURN_VALUE
  bool init(
    int argc
    , char* argv[]
    , const bool auto_start_tracer
    , const std::string event_categories
    , const ::base::FilePath& outDir
    , const ::base::FilePath::CharType icuFileName[]
    , const ::base::FilePath::CharType traceReportFileName[]
    , const int threadsNum);

public:
  ::base::FilePath dir_exe_{};

  ::basic::ScopedLogRunTime scopedLogRunTime{};

  // This object instance is required (for example,
  // LazyInstance, MessageLoop).
  ::base::AtExitManager at_exit{};

  // Build UI thread task executor. This is used by platform
  // implementations for event polling & running background tasks.
  base::SingleThreadTaskExecutor main_task_executor{base::MessagePumpType::UI};

  // allows to schedule arbitrary tasks on main loop
  scoped_refptr<::base::SingleThreadTaskRunner> main_loop_task_runner;

  std::unique_ptr<i18n::I18n> i18n;

  std::unique_ptr<const ::base::FilePath> traceReportPath_;

  // The hang watcher is leaked to make sure it survives all watched threads.
  base::HangWatcher* hang_watcher_;

private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ScopedBaseEnvironment);
};

} // basis
