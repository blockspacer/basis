#pragma once

#include "basic/annotations/guard_annotations.h"

#include <basis/task/periodic_task_executor.h>
#include <basis/task/periodic_check.h>

#include <base/macros.h>
#include <base/sequence_checker.h>
#include <base/callback.h>
#include <base/optional.h>
#include <base/location.h>
#include <base/strings/string_piece.h>
#include <base/threading/thread_collision_warner.h>

#include <basic/rvalue_cast.h>
#include <basic/promise/post_promise.h>

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
// Creates two tasks runners (on `base::ThreadTaskRunnerHandle::Get()`)
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
// ::basis::PeriodicValidateUntil periodicValidateUntil_{};
//
// ::basis::PeriodicValidateUntil::ValidationTaskType validationTask
//   = ::base::BindRepeating(
//       [
//       ](
//         boost::asio::io_context& ioc
//         , ECS::SafeRegistry& asio_registry
//         , /*COPIED*/ ::base::RepeatingClosure resolveCallback
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
//   , ::basis::EndingTimeout{
//       ::base::TimeDelta::FromSeconds(15)} // debug-only expiration time
//   , ::basis::PeriodicCheckUntil::CheckPeriod{
//       ::base::TimeDelta::FromSeconds(1)}
//   , "destruction of allocated connections hanged" // debug-only error
//   , RVALUE_CAST(validationTask)
// )
// .ThenHere(
//   FROM_HERE
//   , ::base::BindOnce(
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
    = ::base::Promise<void, ::base::NoReject>;

  using ValidationTaskType
    = ::base::RepeatingCallback<void(base::RepeatingClosure /*resolveCallback*/)>;

  PeriodicValidateUntil();

  VoidPromise runPromise(
    const ::base::Location& from_here
    , ::basis::EndingTimeout&& debugEndingTimeout
    , ::basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod
    , const std::string& errorText
    , ValidationTaskType&& validationTask);

  bool RunsVerifierInCurrentSequence() const NO_EXCEPTION
  {
    return periodicVerifyRunner_->RunsTasksInCurrentSequence();
  }

  scoped_refptr<::base::SequencedTaskRunner> taskRunner()
  {
    return periodicVerifyRunner_;
  }

 private:
  VoidPromise promiseValidationDone(
    ValidationTaskType&& validationTask
    , ::basis::PeriodicCheckUntil::CheckPeriod&& checkPeriod) NO_EXCEPTION;

 private:
  scoped_refptr<::base::SequencedTaskRunner> periodicVerifyRunner_;

  scoped_refptr<::base::SequencedTaskRunner> timeoutTaskRunner_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PeriodicValidateUntil);
};

} // namespace basis
