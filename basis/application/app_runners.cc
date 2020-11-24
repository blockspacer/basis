#include "basis/application/app_runners.h" // IWYU pragma: associated

#include <string>
#include <utility>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "base/task/post_task.h"
#include "base/task/task_executor.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace application {

namespace {

struct AppRunnerGlobals {
  AppRunnerGlobals() {
    DETACH_FROM_THREAD(main_thread_checker_);
  }

  // AppRunnerGlobals must be initialized on main thread before it's used by
  // any other threads.
  THREAD_CHECKER(main_thread_checker_);

  // |task_runners[id]| is safe to access on |main_thread_checker_| as
  // well as on any thread once it's read-only after initialization
  scoped_refptr<::base::SequencedTaskRunner>
      task_runners[AppRunners::ID_COUNT];
};

AppRunnerGlobals& getAppRunnerGlobals()
{
  static ::base::NoDestructor<AppRunnerGlobals> globals;
  return *globals;
}

// static
const char* getAppRunnerName(AppRunners::ID identifier)
{
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, AppRunners::ID_COUNT);

  static const char* const kThreadRunnerNames[AppRunners::ID_COUNT] = {
#if defined(ENABLE_APP_UI_RUNNER)
    "UI_ThreadRunner"
    ,
#endif // defined(ENABLE_APP_UI_RUNNER)
    "FIXED_LOOP_ThreadRunner"
    , "ENTT_ThreadRunner"
#if defined(ENABLE_APP_NON_BLOCK_IO_RUNNER)
    , "NON_BLOCK_IO_ThreadRunner"
#endif // defined(ENABLE_APP_NON_BLOCK_IO_RUNNER)
  };

  if (identifier > 0 && identifier < AppRunners::ID_COUNT) {
    return kThreadRunnerNames[identifier];
  }

  return "Unknown Thread";
}

}  // namespace

// static
scoped_refptr<::base::SequencedTaskRunner>
AppRunners::getTaskRunner(AppRunners::ID identifier)
{
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, AppRunners::ID_COUNT);

  AppRunnerGlobals& globals = getAppRunnerGlobals();

  DCHECK_CALLED_ON_VALID_THREAD(globals.main_thread_checker_);

  DCHECK(globals.task_runners[identifier]);
  return globals.task_runners[identifier];
}

// static
void
AppRunners::registerGlobalTaskRunner(const AppRunners::ID& identifier
      , scoped_refptr<::base::SequencedTaskRunner> task_runner)
      {
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, AppRunners::ID_COUNT);

  AppRunnerGlobals& globals = getAppRunnerGlobals();

  DCHECK_CALLED_ON_VALID_THREAD(globals.main_thread_checker_);

  DCHECK(!globals.task_runners[identifier]);
  globals.task_runners[identifier] = task_runner;
}

// static
bool
AppRunners::CurrentlyOn(ID identifier)
{
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, ID_COUNT);

  AppRunnerGlobals& globals = getAppRunnerGlobals();

  DCHECK(globals.task_runners[identifier]);

  // Thread-safe since |globals.task_runners| is read-only after being
  // initialized from main thread runner
  return globals.task_runners[identifier] &&
         globals.task_runners[identifier]->RunsTasksInCurrentSequence();
}

// static
std::string AppRunners::GetDCheckCurrentlyOnErrorMessage(ID identifier)
{
  std::string actual_name = ::base::PlatformThread::GetName();
  if (actual_name.empty()) {
    actual_name = "Unknown Thread Runer";
  }

  std::string result = "Must be called on ";
  result += getAppRunnerName(identifier);
  result += "; actually called on ";
  result += actual_name;
  result += ".";
  return result;
}

bool runOrPostTaskOn(const ::base::Location& location
  , AppRunners::ID id
  , ::base::OnceClosure task)
{
  if (AppRunners::CurrentlyOn(id))
  {
    std::move(task).Run();
    return true;
  }

  return AppRunners::getTaskRunner(id)
    ->PostTask(location, std::move(task));
}

}  // namespace application
