#pragma once

#if 0
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

/// \note ScopedCrossSequenceCtxVar is async (uses `PostTask`)
/// alternative to `basis::ScopedSequenceCtxVar` (without `reset()` method)
/// because it allows to modify storage of one sequence
/// from another sequence.
/// If you need to change context of same sequence, than
/// prefer to use `basis::ScopedSequenceCtxVar` or `ECS::SequenceLocalContext`.
///
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
//     ScopedCrossSequenceCtxVar<AsioUpdater> scopedSequencedAsioExecutor
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
//         , base::IsNestedPromise{true}
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
class ScopedCrossSequenceCtxVar
{
 public:
  using VoidPromise
    = base::Promise<void, base::NoReject>;

  using CtxTypePromise
    = base::Promise<Type*, base::NoReject>;

  ScopedCrossSequenceCtxVar(
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

  ~ScopedCrossSequenceCtxVar()
  {
    LOG_CALL(DVLOG(99));

    DCHECK_RUN_ON(&sequence_checker_);

    taskRunner_->PostTask(FROM_HERE
      , base::BindOnce(
          &destructScopedCrossSequenceCtxVar
          , taskRunner_ // prolong lifetime
          , base::Passed(destructionResolver_.GetRepeatingResolveCallback())
      )
    );
  }

  /// \note can be called AFTER destructor finished, so method must be `static`
  /// i.e. unable use member variables etc.
  static void destructScopedCrossSequenceCtxVar(
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
  /// if `destructScopedCrossSequenceCtxVar` finished
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
          &ScopedCrossSequenceCtxVar::emplace<Args&& ...>
          , base::Unretained(this)
          , from_here
          , debug_name + "_" + FROM_HERE.ToString()
          , std::forward<Args>(args)...
        )
    );
  }

  SET_WEAK_SELF(ScopedCrossSequenceCtxVar)

 private:
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

 private:
  // provider of sequence-local-storage
  scoped_refptr<base::SequencedTaskRunner>
    taskRunner_;

  // resolved in `destructVar()`
  base::ManualPromiseResolver<void, base::NoReject>
    destructionResolver_;

  SEQUENCE_CHECKER(sequence_checker_);

  SET_WEAK_POINTERS(ScopedCrossSequenceCtxVar);

  DISALLOW_COPY_AND_ASSIGN(ScopedCrossSequenceCtxVar);
};

// Calls `reset()` (if value exists) upon scope exit.
//
// Manipulates local storage of sequence that was
// used for object construction
// (calls `base::SequencedTaskRunnerHandle::Get()`).
//
// Construction, destruction and all methods
// must be used on same sequence.
//
/// \note It also behaves like `base::Optional`
/// i.e. has `emplace()` and `reset()` methods.
//
// USAGE
//
//   // in main source file: create prev. not-existing `SequenceCtxVar`
//   basis::ScopedSequenceCtxVar<
//     backend::ConsoleTerminalEventDispatcher
//   > consoleTerminalEventDispatcher_;
//
//   ignore_result(
//    consoleTerminalEventDispatcher_.emplace(
//      FROM_HERE
//      , "ConsoleTerminalEventDispatcher_" + FROM_HERE.ToString()
//    )
//   );
//
//   consoleTerminalEventDispatcher_->doSmth(...);
//
//   // in other source file: get already existing `SequenceCtxVar`
//   base::WeakPtr<ECS::SequenceLocalContext> mainLoopContext_{
//        ECS::SequenceLocalContext::getSequenceLocalInstance(
//          FROM_HERE, base::MessageLoop::current()->task_runner())};
//
//    util::UnownedRef<
//      ::backend::ConsoleTerminalEventDispatcher
//    > consoleTerminalEventDispatcher_{
//        REFERENCED(mainLoopContext_->ctx<
//          ::backend::ConsoleTerminalEventDispatcher
//        >(FROM_HERE))};
template<typename Type>
class ScopedSequenceCtxVar
{
 public:
  ScopedSequenceCtxVar()
    : ALLOW_THIS_IN_INITIALIZER_LIST(
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

    reset();
  }

  void reset()
  {
    LOG_CALL(DVLOG(99));

    DCHECK_RUN_ON(&sequence_checker_);

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);
    if(sequenceLocalContext->try_ctx<Type>(FROM_HERE))
    {
      sequenceLocalContext->unset<Type>(FROM_HERE);
    }
  }

  template <class... Args>
  MUST_USE_RETURN_VALUE
  Type* emplace(
    const base::Location& from_here
    , const std::string& debug_name
    , Args&&... args) NO_EXCEPTION
  {
    LOG_CALL(DVLOG(99));

    DCHECK_RUN_ON(&sequence_checker_);

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

  MUST_USE_RETURN_VALUE
  Type& get() NO_EXCEPTION
  {
    DCHECK_RUN_ON(&sequence_checker_);

    base::WeakPtr<ECS::SequenceLocalContext> sequenceLocalContext
      = ECS::SequenceLocalContext::getSequenceLocalInstance(
          FROM_HERE, base::SequencedTaskRunnerHandle::Get());

    DCHECK(sequenceLocalContext);

    DCHECK(sequenceLocalContext->try_ctx<Type>(FROM_HERE));

    return sequenceLocalContext->ctx<Type>(FROM_HERE);
  }

  constexpr Type& operator*()
  {
    return get();
  }

  constexpr Type& operator*() const
  {
    return get();
  }

  constexpr Type* operator->()
  {
    return &get();
  }

  constexpr Type* operator->() const
  {
    return &get();
  }

  SET_WEAK_SELF(ScopedSequenceCtxVar)

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  SET_WEAK_POINTERS(ScopedSequenceCtxVar);

  DISALLOW_COPY_AND_ASSIGN(ScopedSequenceCtxVar);
};

} // namespace basis
#endif // 0
