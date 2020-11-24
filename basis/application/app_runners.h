#pragma once

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"

// Use DCHECK_CURRENTLY_ON_RUNNER(AppRunner::ID) to assert that a function can only
// be called on the named AppRunner.
#define DCHECK_CURRENTLY_ON_RUNNER(identifier)                              \
  (DCHECK(::application::AppRunners::CurrentlyOn(identifier))        \
   << ::application::AppRunners::GetDCheckCurrentlyOnErrorMessage(   \
          identifier))

// Usage: DCHECK_ON_RUNNER(ENTT)
#define DCHECK_ON_RUNNER(identifier)               \
  DCHECK_CURRENTLY_ON_RUNNER(::application::AppRunners:: identifier)

// Usage: RUNNER_BY_ID(::application::AppRunners::ENTT)
#define RUNNER_BY_ID(identifier)               \
  (::application::AppRunners::getTaskRunner(   \
     (identifier)))

// Usage: APP_RUNNER(ENTT)
#define APP_RUNNER(identifier)               \
  RUNNER_BY_ID(::application::AppRunners:: identifier)

namespace application {

class AppRunnersImpl;

/// \todo runtime registration of SequencedTaskRunner
/// by hashed string id and hash map

/// \todo rename to app runners

/// \note runner is not same as thread (just sequence)
/// see https://github.com/chromium/chromium/blob/master/docs/threading_and_tasks.md

class AppRunners {
 public:
  // An enumeration of the well-known threads.
  enum ID {
#if defined(ENABLE_APP_UI_RUNNER)
    // The main thread runner. It stops running tasks during shutdown
    // and is never joined.
    UI,
#endif // defined(ENABLE_APP_UI_RUNNER)

    // fixed time step loop
    FIXED_LOOP,

    // we use entt for ECS
    ENTT,

#if defined(ENABLE_APP_NON_BLOCK_IO_RUNNER)
    // This is the thread runner that processes non-blocking I/O, i.e. IPC and network.
    // Blocking I/O should happen in ::base::ThreadPool. It is joined on shutdown
    // (and thus any task posted to it may block shutdown).
    NON_BLOCK_IO,
#endif // defined(ENABLE_APP_NON_BLOCK_IO_RUNNER)

    // NOTE: do not add new threads here. Instead you should just use
    // ::base::ThreadPool::Create*TaskRunner to run tasks on the ::base::ThreadPool.

    // This identifier does not represent a thread.  Instead it counts the
    // number of well-known threads.  Insert new well-known threads before this
    // identifier.
    ID_COUNT
  };

 public:
  static scoped_refptr<::base::SequencedTaskRunner>
    getTaskRunner(
      AppRunners::ID identifier) WARN_UNUSED_RESULT;

  // register task runner globally with key
  static void
    registerGlobalTaskRunner(const AppRunners::ID& identifier
      , scoped_refptr<::base::SequencedTaskRunner> task_runner);

  // Callable on any thread runner.
  // Returns whether you're currently on a particular thread runner.
  // To DCHECK this, use the DCHECK_CURRENTLY_ON() macro above.
  static bool
    CurrentlyOn(ID identifier) WARN_UNUSED_RESULT;

  // Returns an appropriate error message for when DCHECK_CURRENTLY_ON() fails.
  static std::string
    GetDCheckCurrentlyOnErrorMessage(ID identifier) WARN_UNUSED_RESULT;

 private:
  friend class AppRunnersImpl;
  AppRunners() = default;

  DISALLOW_COPY_AND_ASSIGN(AppRunners);
};

// Runs |task| on the thread runner specified by |id|
// if already on that thread runner,
// otherwise posts a task to that thread runner.
//
// Returns true if the task may be
// run at some point in the future, and false if the task definitely
// will not be run.
bool runOrPostTaskOn(const ::base::Location& location
  , AppRunners::ID id
  , ::base::OnceClosure task);

}  // namespace application
