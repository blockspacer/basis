#pragma once

#include "basis/lock_with_check.hpp"
#include "basis/ECS/sequence_local_context.hpp"

#include <base/macros.h>
#include <base/sequence_checker.h>
#include <base/callback.h>
#include <base/optional.h>
#include <base/location.h>
#include <base/logging.h>
#include <base/rvalue_cast.h>
#include <base/bind_helpers.h>
#include <base/strings/string_piece.h>
#include <base/threading/thread_collision_warner.h>

#include <basis/bitmask.h>
#include <basis/promise/post_promise.h>

#include <functional>
#include <map>
#include <string>

namespace basis {

// Store data type in context that bound to single sequence.
// Uses RAII to free stored variable on scope exit.
//
/// \note Manipulation of sequence-local-storage is asyncronous,
/// so you must wait for construction/deletion or use `base::Promise`
//
// USAGE
//
//   {
//     // Create UNIQUE type to store in sequence-local-context
//     /// \note initialized, used and destroyed
//     /// on `periodicAsioTaskRunner_` sequence-local-context
//     using AsioUpdater
//       = util::StrongAlias<
//           class PeriodicAsioExecutorTag
//           /// \note will stop periodic timer on scope exit
//           , basis::PeriodicTaskExecutor
//         >;
//
//     /// \note Will free stored variable on scope exit.
//     ScopedSequenceCtxVar<AsioUpdater> scopedSequencedAsioExecutor
//       (periodicAsioTaskRunner_);
//
//     VoidPromise emplaceDonePromise
//       = scopedSequencedAsioExecutor.emplace_async(FROM_HERE
//             , "PeriodicAsioExecutor" // debug name
//             , base::BindRepeating(
//                 &ExampleServer::updateAsioRegistry
//                 , base::Unretained(this))
//       )
//     .ThenOn(periodicAsioTaskRunner_
//       , FROM_HERE
//       , base::BindOnce(
//         []
//         (
//           scoped_refptr<base::SequencedTaskRunner>
//             periodicAsioTaskRunner
//           , AsioUpdater* periodicAsioExecutor
//         ){
//           LOG_CALL(DVLOG(99));
//
//           /// \todo make period configurable
//           (*periodicAsioExecutor)->startPeriodicTimer(
//             base::TimeDelta::FromMilliseconds(100));
//
//           DCHECK(periodicAsioTaskRunner->RunsTasksInCurrentSequence());
//         }
//         , periodicAsioTaskRunner_
//       )
//     );
//     /// \note Will block current thread for unspecified time.
//     base::waitForPromiseResolve(FROM_HERE, emplaceDonePromise);
//
//     promiseRunDone =
//       promiseRunDone
//       .ThenOn(periodicAsioTaskRunner_
//         , FROM_HERE
//         , scopedSequencedAsioExecutor.promiseDeletion()
//         , /*nestedPromise*/ true
//       );
//
//     run_loop_.Run();
//   }
//
//   // Wait for deletion  of `periodicAsioExecutor_`
//   // on task runner associated with it
//   // otherwise you can get `use-after-free` error
//   // on resources bound to periodic callback.
//   /// \note We can not just call `periodicAsioTaskRunner_.reset()` because
//   /// it is shared type and `PeriodicAsioExecutor` prolongs its lifetime.
//   /// \note Will block current thread for unspecified time.
//   base::waitForPromiseResolve(FROM_HERE, promiseRunDone);
template<typename Type>
class ScopedSequenceCtxVar
{
 public:
  using VoidPromise
    = base::Promise<void, base::NoReject>;

  using CtxTypePromise
    = base::Promise<Type*, base::NoReject>;

  ScopedSequenceCtxVar(
    scoped_refptr<base::SequencedTaskRunner> taskRunner)
    : taskRunner_(taskRunner)
    , destructionResolver_(FROM_HERE)
    , ALLOW_THIS_IN_INITIALIZER_LIST(
        weak_ptr_factory_(COPIED(this)))
    , ALLOW_THIS_IN_INITIALIZER_LIST(
        weak_this_(
          weak_ptr_factory_.GetWeakPtr()))
  {
    LOG_CALL(DVLOG(99));

    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  ~ScopedSequenceCtxVar()
  {
    LOG_CALL(DVLOG(99));

    DCHECK_RUN_ON(&sequence_checker_);

    taskRunner_->PostTask(FROM_HERE
      , base::BindOnce(
          &destructScopedSequenceCtxVar
          , taskRunner_ // prolong lifetime
          , base::Passed(destructionResolver_.GetRepeatingResolveCallback())
      )
    );
  }

  /// \note can be called AFTER destructor finished, so method must be `static`
  /// i.e. unable use member variables etc.
  static void destructScopedSequenceCtxVar(
    scoped_refptr<base::SequencedTaskRunner> taskRunner
    , base::OnceClosure resolveCb) NO_EXCEPTION
  {
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner.get());

    base::rvalue_cast(resolveCb).Run();

    LOG_CALL(DVLOG(99));

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);
    DCHECK(sequenceLocalContext->try_ctx<Type>(FROM_HERE));
    sequenceLocalContext->unset<Type>(FROM_HERE);
  }

  /// \note API is asyncronous, so you must check
  /// if `destructScopedSequenceCtxVar` finished
  MUST_USE_RETURN_VALUE
  VoidPromise promiseDeletion() NO_EXCEPTION
  {
    LOG_CALL(DVLOG(99));

    return destructionResolver_.promise();
  }

  template <class... Args>
  MUST_USE_RETURN_VALUE
  CtxTypePromise emplace_async(
    const base::Location& from_here
    , const std::string& debug_name
    , Args&&... args) NO_EXCEPTION
  {
    LOG_CALL(DVLOG(99));

    DCHECK(!taskRunner_->RunsTasksInCurrentSequence());
    return base::PostPromise(FROM_HERE
      , taskRunner_.get()
      , base::BindOnce(
          &ScopedSequenceCtxVar::emplace<Args ...>
          , base::Unretained(this)
          , from_here
          , debug_name + "_" + FROM_HERE.ToString()
          , std::forward<Args>(args)...
        )
    );
  }

  template <class... Args>
  MUST_USE_RETURN_VALUE
  Type* emplace(
    const base::Location& from_here
    , const std::string& debug_name
    , Args&&... args) NO_EXCEPTION
  {
    LOG_CALL(DVLOG(99));

    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);
    // Can not register same data type twice.
    // Forces users to call `sequenceLocalContext->unset`.
    DCHECK(!sequenceLocalContext->try_ctx<Type>(FROM_HERE));
    Type& result
      = sequenceLocalContext->set_once<Type>(
          from_here
          , debug_name + "_" + FROM_HERE.ToString()
          , std::forward<Args>(args)...
        );
    return &result;
  }

  SET_WEAK_SELF(ScopedSequenceCtxVar)

 private:
  // provider of sequence-local-storage
  scoped_refptr<base::SequencedTaskRunner>
    taskRunner_;

  // resolved in `destructVar()`
  base::ManualPromiseResolver<void, base::NoReject>
    destructionResolver_;

  SEQUENCE_CHECKER(sequence_checker_);

  SET_WEAK_POINTERS(ScopedSequenceCtxVar);

  DISALLOW_COPY_AND_ASSIGN(ScopedSequenceCtxVar);
};

} // namespace basis
