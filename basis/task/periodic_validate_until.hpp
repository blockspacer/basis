#pragma once

#include "basis/lock_with_check.hpp"

#include <base/macros.h>
#include <base/sequence_checker.h>
#include <base/callback.h>
#include <base/optional.h>
#include <base/location.h>
#include <base/rvalue_cast.h>
#include <base/bind_helpers.h>
#include <base/strings/string_piece.h>
#include <base/threading/thread_collision_warner.h>

#include <basis/promise/post_promise.h>
#include <basis/task/periodic_task_executor.hpp>
#include <basis/task/periodic_check.hpp>

#include <functional>
#include <map>
#include <string>

namespace basis {

/// \note Make sure `validation task` will succeed someday
/// because `execution time` will be limited only in DEBUG builds.
//
// Runs `validation task` periodically until it returs true
// (validation will run as long as `validation task` returns false).
// Task execution time is limited (will DCHECK in case of error).
//
// Creates two tasks runners (on `base::ThreadPool::GetInstance()`)
// and uses their sequence-local contexts:
// 1. Creates task runner to run `validation task` periodically.
// Uses `basis::startPeriodicTaskExecutorOnSequence`.
// 2. Creates task runner to check for expiration time periodically.
// Uses `basis::setPeriodicTimeoutCheckerOnSequence.
//
// PERFORMANCE
//
// Performance overhead expected to be NOT large (TODO: measure).
// Designed for NOT performance-critical code.
// Uses `base::Promise` (i.e. dynamic allocations),
// so avoid it in hot-code-paths.
//
// USAGE
//
// basis::PeriodicValidateUntil periodicValidateUntil_{};
//
// basis::PeriodicValidateUntil::ValidationTaskType validationTask
//   = base::BindRepeating(
//       [
//       ](
//         boost::asio::io_context& ioc
//         , ECS::AsioRegistry& asio_registry
//         , COPIED() base::RepeatingClosure resolveCallback
//       ){
//         LOG(INFO)
//           << "waiting for cleanup of asio registry...";
//
//         // ...
//         // redirect task to strand
//         // ...
//       }
//       , REFERENCED(ioc_)
//       , REFERENCED(asioRegistry_)
//   );
//
// return periodicValidateUntil_.runPromise(FROM_HERE
//   , basis::EndingTimeout{
//       base::TimeDelta::FromSeconds(15)} // debug-only expiration time
//   , basis::PeriodicCheckUntil::CheckPeriod{
//       base::TimeDelta::FromSeconds(1)}
//   , "destruction of allocated connections hanged" // debug-only error
//   , base::rvalue_cast(validationTask)
// )
// .ThenHere(
//   FROM_HERE
//   , base::BindOnce(
//     []
//     ()
//     {
//       LOG(INFO)
//         << "finished cleanup of network entities";
//     }
//   )
// );
class PeriodicValidateUntil {
 public:
  using VoidPromise
    = base::Promise<void, base::NoReject>;

  using ValidationTaskType
    = base::RepeatingCallback<void(base::RepeatingClosure /*resolveCallback*/)>;

  PeriodicValidateUntil();

  VoidPromise runPromise(
    const base::Location& from_here
    , basis::EndingTimeout&& debugEndingTimeout
    , basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod
    , const std::string& errorText
    , ValidationTaskType&& validationTask);

  bool RunsVerifierInCurrentSequence() const NO_EXCEPTION
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(periodicVerifyRunner_));

    return periodicVerifyRunner_->RunsTasksInCurrentSequence();
  }

  scoped_refptr<base::SequencedTaskRunner> taskRunner()
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(periodicVerifyRunner_));

    return periodicVerifyRunner_;
  }

 private:
  VoidPromise promiseValidationDone(
    ValidationTaskType&& validationTask
    , basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod) NO_EXCEPTION;

 private:
  scoped_refptr<base::SequencedTaskRunner> periodicVerifyRunner_
    SET_STORAGE_THREAD_GUARD(MEMBER_GUARD(periodicVerifyRunner_));

  scoped_refptr<base::SequencedTaskRunner> timeoutTaskRunner_
    SET_STORAGE_THREAD_GUARD(MEMBER_GUARD(timeoutTaskRunner_));

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PeriodicValidateUntil);
};

} // namespace basis