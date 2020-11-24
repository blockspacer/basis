#pragma once

#include "basis/verify_nothing.hpp"
#include "basis/scoped_checks.hpp"

#include <base/macros.h>
#include <base/sequence_checker.h>
#include <base/callback.h>
#include <base/optional.h>
#include <base/location.h>
#include <base/rvalue_cast.h>
#include <base/bind_helpers.h>
#include <base/strings/string_piece.h>
#include <base/threading/thread_collision_warner.h>
#include <base/sequenced_task_runner.h>
#include <base/thread_annotations.h>

#include <basis/bitmask.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include <functional>
#include <map>
#include <string>

// -----------------------------------------------------------------------------
// Usage documentation
// -----------------------------------------------------------------------------
//
// Overview:
// We can use approach similar to `base::AutoLock`
// to perform checks on scope entry (and/or exit) and require 'custom locks'
// in code (guard code) using Clang Thread-Safety annotations
// i.e. we create custom types like `base::AutoLockRunOnThreadId myThreadIdLock_`
// and can use annotations like `GUARDED_BY(myThreadIdLock_)`.
//
// MOTIVATION
//
// We use Clang Thread-Safety annotations to:
//
// * Guard usage of some data or method with custom checks (like memory validity)
//   See `GUARD_MEMBER_WITH_CHECK`
// * Guard some data or method with thread id (single-thread-bound)
//   sequence id (sequence-bound), strand id check (asio strand-bound), etc.
//   See `PRIVATE_METHOD_RUN_ON(&sequence_checker_)`,
//  `PUBLIC_METHOD_RUN_ON(&perConnectionStrand_)` etc.
// * Document that you must take care of thread-safety while using some data or method
//   See `CREATE_METHOD_GUARD`, `GUARD_MEMBER_OF_UNKNOWN_THREAD`,
//   `GUARD_METHOD_ON_UNKNOWN_THREAD`, `DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD`,
//   `DCHECK_MEMBER_OF_UNKNOWN_THREAD` etc.

namespace basis {

// Allows to use `Type` with clang thread-safety annotations like `GUARDED_BY`.
// See http://clang.llvm.org/docs/ThreadSafetyAnalysis.html
template<typename Type>
struct LOCKABLE AnnotateLockable
{
  template <class... Args>
  AnnotateLockable(
    Args&&... args)
    : data(std::forward<Args>(args)...)
  {}

  constexpr const Type& operator*() const
  {
    return data;
  }

  constexpr Type& operator*()
  {
    return data;
  }

  constexpr const Type* operator->() const
  {
    return &data;
  }

  constexpr Type* operator->()
  {
    return &data;
  }

  using StoredType = Type;

  Type data;
};

// Helper class used by DCHECK_RUN_ON
class SCOPED_LOCKABLE SequenceCheckerScope {
 public:
  explicit SequenceCheckerScope(
    const ::base::SequenceChecker* thread_like_object)
      EXCLUSIVE_LOCK_FUNCTION(thread_like_object) {}

  SequenceCheckerScope(
    const SequenceCheckerScope&) = delete;

  SequenceCheckerScope& operator=(
    const SequenceCheckerScope&) = delete;

  ~SequenceCheckerScope() UNLOCK_FUNCTION() {}

  static bool CalledOnValidSequence(
    const ::base::SequenceChecker* thread_like_object)
  {
    return thread_like_object->CalledOnValidSequence();
  }
};

// Helper class used by DCHECK_RUN_ON
class SCOPED_LOCKABLE SequencedTaskRunnerScope {
 public:
  explicit SequencedTaskRunnerScope(
    const ::base::SequencedTaskRunner* thread_like_object)
      EXCLUSIVE_LOCK_FUNCTION(thread_like_object) {}

  SequencedTaskRunnerScope(
    const SequencedTaskRunnerScope&) = delete;

  SequencedTaskRunnerScope& operator=(
    const SequencedTaskRunnerScope&) = delete;

  ~SequencedTaskRunnerScope() UNLOCK_FUNCTION() {}

  static bool RunsTasksInCurrentSequence(
    const ::base::SequencedTaskRunner* thread_like_object)
  {
    return thread_like_object->RunsTasksInCurrentSequence();
  }
};

// Allows to use `boost::asio::strand` with clang thread-safety annotations like `GUARDED_BY`.
// See http://clang.llvm.org/docs/ThreadSafetyAnalysis.html
template <typename Executor>
using AnnotatedStrand
  = ::basis::AnnotateLockable<
      ::boost::asio::strand<Executor>
    >;

// Helper class used by DCHECK_RUN_ON_STRAND
class SCOPED_LOCKABLE StrandCheckerScope {
 public:
  template <typename Executor>
  explicit StrandCheckerScope(
    const ::basis::AnnotateLockable<boost::asio::strand<Executor>>* thread_like_object)
      EXCLUSIVE_LOCK_FUNCTION(thread_like_object) {}

  StrandCheckerScope(
    const StrandCheckerScope&) = delete;

  StrandCheckerScope& operator=(
    const StrandCheckerScope&) = delete;

  ~StrandCheckerScope() UNLOCK_FUNCTION() {}
};

#define CGEN_CAT(a, b) CGEN_CAT_I(a, b)
#define CGEN_CAT_I(a, b) CGEN_CAT_II(~, a ## b)
#define CGEN_CAT_II(p, res) res

#define CGEN_UNIQUE_NAME(base) CGEN_CAT(base, __COUNTER__)

// RUN_ON/GUARDED_BY/DCHECK_RUN_ON macros allows to annotate
// variables are accessed from same thread/task queue.
// Using tools designed to check mutexes, it checks at compile time everywhere
// variable is access, there is a run-time dcheck thread/task queue is correct.
//
// class ThreadExample {
//  public:
//   void NeedVar1() {
//     DCHECK_RUN_ON(network_thread_);
//     transport_->Send();
//   }
//
//  private:
//   Thread* network_thread_;
//   int transport_ GUARDED_BY(network_thread_);
// };
//
// class SequenceCheckerExample {
//  public:
//   int CalledFromPacer() RUN_ON(pacer_sequence_checker_) {
//     return var2_;
//   }
//
//   void CallMeFromPacer() {
//     DCHECK_RUN_ON(&pacer_sequence_checker_)
//        << "Should be called from pacer";
//     CalledFromPacer();
//   }
//
//  private:
//   int pacer_var_ GUARDED_BY(pacer_sequence_checker_);
//   SequenceChecker pacer_sequence_checker_;
// };
//
// class TaskQueueExample {
//  public:
//   class Encoder {
//    public:
//     TaskQueue* Queue() { return encoder_queue_; }
//     void Encode() {
//       DCHECK_RUN_ON(encoder_queue_);
//       DoSomething(var_);
//     }
//
//    private:
//     TaskQueue* const encoder_queue_;
//     Frame var_ GUARDED_BY(encoder_queue_);
//   };
//
//   void Encode() {
//     // Will fail at runtime when DCHECK is enabled:
//     // encoder_->Encode();
//     // Will work:
//     scoped_refptr<Encoder> encoder = encoder_;
//     encoder_->Queue()->PostTask([encoder] { encoder->Encode(); });
//   }
//
//  private:
//   scoped_refptr<Encoder> encoder_;
// }

// Document if a function expected to be called from same thread/task queue.
/// \note use it with functions in `private` API (that can NOT call `DCHECK_RUN_ON*`)
//
// EXAMPLE
//
// class MyClass {
// protected:
//  void internalRunLoop() NO_EXCEPTION
//    RUN_ON(&sequence_checker_);
// };
#define THREAD_ANNOTATION_ATTRIBUTE__RUN_ON(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(__VA_ARGS__))

#define PRIVATE_METHOD_RUN_ON(...) \
  THREAD_ANNOTATION_ATTRIBUTE__RUN_ON(__VA_ARGS__)

#define PROTECTED_METHOD_RUN_ON(...) \
  THREAD_ANNOTATION_ATTRIBUTE__RUN_ON(__VA_ARGS__)

/// \note use it with functions in `public` API (that can call `DCHECK_RUN_ON*`)
//
// EXAMPLE
//
// class MyClass {
// public:
//  void runLoop() NO_EXCEPTION
//    THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_LOCKS_EXCLUDED(&sequence_checker_);
// };
#define THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_LOCKS_EXCLUDED(...) \
  LOCKS_EXCLUDED(__VA_ARGS__)

#define PUBLIC_METHOD_RUN_ON(...) \
  THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_LOCKS_EXCLUDED(__VA_ARGS__)

#define ANNOTATE_NOT_RUN_ON(...) \
  LOCKS_EXCLUDED(__VA_ARGS__)

// Type of `x` is `base::SequenceChecker*`
#define DCHECK_RUN_ON(x)                                              \
  ::basis::SequenceCheckerScope seq_check_scope(x); \
  DCHECK((x)); \
  DCHECK((x)->CalledOnValidSequence())

// Type of `x` is `base::SequencedTaskRunner*`
//
// USAGE
//
// scoped_refptr<::base::SequencedTaskRunner> periodicVerifyRunner_
//   // It safe to read value from any thread because its storage
//   // expected to be not modified (if properly initialized)
//   GUARD_WITH_FAKE_LOCK(periodicVerifyRunner_);
// // ...
// DCHECK_THREAD_GUARD_SCOPE(periodicVerifyRunner_);
// DCHECK(periodicVerifyRunner_);
// DCHECK_RUN_ON_SEQUENCED_RUNNER(periodicVerifyRunner_.get());
#define DCHECK_RUN_ON_SEQUENCED_RUNNER(x)                                              \
  ::basis::SequencedTaskRunnerScope seq_task_runner_scope(x); \
  DCHECK((x)); \
  DCHECK((x)->RunsTasksInCurrentSequence())

// Type of `x` is `basis::AnnotatedStrand&`
//
// EXAMPLE
//
// using ExecutorType
//   = StreamType::executor_type;
//
// using StrandType
//   = ::boost::asio::strand<ExecutorType>;
//
// // |stream_| and calls to |async_*| are guarded by strand
// ::basis::AnnotatedStrand<ExecutorType> perConnectionStrand_
//   SET_CUSTOM_THREAD_GUARD_WITH_CHECK(
//     perConnectionStrand_
//     // 1. It safe to read value from any thread
//     // because its storage expected to be not modified.
//     // 2. On each access to strand check that stream valid
//     // otherwise `::boost::asio::post` may fail.
//     , ::base::BindRepeating(
//       [
//       ](
//         bool is_stream_valid
//         , StreamType& stream
//       ){
//         /// \note |perConnectionStrand_|
//         /// is valid as long as |stream_| valid
//         /// i.e. valid util |stream_| moved out
//         /// (it uses executor from stream).
//         return is_stream_valid;
//       }
//       , is_stream_valid_.load()
//       , REFERENCED(stream_.value())
//     ));
//
// DCHECK_RUN_ON_STRAND(&perConnectionStrand_, ExecutorType);
//
#define DCHECK_RUN_ON_STRAND(x, Type)                                              \
  ::basis::StrandCheckerScope strand_check_scope(x); \
  DCHECK((x)); \
  DCHECK((x)->data.running_in_this_thread())

// Will call `callback_.Run()` in any builds (including release),
// so take care of performance
struct FakeLockPolicyAlways
{
  /// \todo refactor to `enum class { isDebug, isSkip, isAlways }`
  static constexpr bool isDebugOnly = false;
  static constexpr bool isSkip = false;
  static constexpr bool isAlways = true;
};

// Will call `callback_.Run()` only in debug builds,
// prefer for performance reasons
struct FakeLockPolicyDebugOnly
{
  /// \todo refactor to `enum class { isDebug, isSkip, isAlways }`
  static constexpr bool isDebugOnly = true;
  static constexpr bool isSkip = false;
  static constexpr bool isAlways = false;
};

/// \note avoid `FakeLockPolicySkip` if you can
// Can be used to implement custom verification logic
struct FakeLockPolicySkip
{
  /// \todo refactor to `enum class { isDebug, isSkip, isAlways }`
  static constexpr bool isDebugOnly = false;
  static constexpr bool isSkip = true;
  static constexpr bool isAlways = false;
};

struct FakeLockCheckWholeScope
{
  /// \todo refactor to `enum class`
  static constexpr bool isWholeScope = true;
  static constexpr bool isEnterScope = false;
  static constexpr bool isExitScope = false;
};

struct FakeLockCheckEnterScope
{
  /// \todo refactor to `enum class`
  static constexpr bool isWholeScope = false;
  static constexpr bool isEnterScope = true;
  static constexpr bool isExitScope = false;
};

struct FakeLockCheckExitScope
{
  /// \todo refactor to `enum class`
  static constexpr bool isWholeScope = false;
  static constexpr bool isEnterScope = false;
  static constexpr bool isExitScope = true;
};

template <
  typename Signature
>
class FakeLockWithCheck;

template <
  // FakeLockCheckType::isDebugOnly performs check only in debug builds
  // FakeLockCheckType::isSkip does not perform any checks
  // FakeLockCheckType::isAlways performs checks in all builds
  typename FakeLockPolicyType
  // FakeLockCheckType::isWholeScope performs check on both scope enter and exit
  // FakeLockCheckType::isEnterScope performs check only on scope enter
  // FakeLockCheckType::isEnterScope performs check only on scope exit
  , typename FakeLockCheckType
  , typename Signature
>
class ScopedFakeLockWithCheck;

/// \note Prefer instead `DCHECK_RUN_ON` to `FakeLockWithCheck` where possible.
/// \note It is not real lock, only annotated as lock.
/// It just calls callback on scope entry AND exit.
/// \note Need to build with `-Wthread-safety-analysis`
/// flag to see some effect.
/// see https://pspdfkit.com/blog/2020/the-cpp-lifetime-profile/
/// see http://clang.llvm.org/docs/ThreadSafetyAnalysis.html
/// see https://github.com/isocpp/CppCoreGuidelines/blob/master/docs/Lifetime.pdf
template<
  typename R
  , typename... Args
>
class LOCKABLE
  FakeLockWithCheck<R(Args...)>
{
 public:
  using RunType = R(Args...);

  FakeLockWithCheck(
    const ::base::RepeatingCallback<RunType>& callback)
    : callback_(callback) {}

  FakeLockWithCheck(
    ::base::RepeatingCallback<RunType>&& callback)
    : callback_(base::rvalue_cast(callback)) {}

  MUST_USE_RETURN_VALUE
  bool Acquire() const NO_EXCEPTION EXCLUSIVE_LOCK_FUNCTION()
  {
    DCHECK(callback_);
    return callback_.Run();
  }

  MUST_USE_RETURN_VALUE
  bool Release() const NO_EXCEPTION UNLOCK_FUNCTION()
  {
    DCHECK(callback_);
    return callback_.Run();
  }

 private:
  ::base::RepeatingCallback<RunType> callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeLockWithCheck);
};

// Will call `FakeLockWithCheck::callback_.Run()`
// on scope entry AND exit.
//
// USAGE
//  class MyClass {
//    // ...
//
//    using FakeLockRunType = bool();
//
//    using FakeLockPolicy = ::basis::FakeLockPolicyDebugOnly;
//
//    MUST_USE_RETURN_VALUE
//    ::base::WeakPtr<Listener> weakSelf() const NO_EXCEPTION
//    {
//      /// \note `FakeLockPolicySkip` will NOT perform any checks.
//      /// No need to check thread-safety because `weak_this_`
//      /// can be passed safely between threads if not modified.
//      ::basis::ScopedFakeLockWithCheck<basis::FakeLockPolicySkip, FakeLockRunType>
//        auto_lock(fakeLockToSequence_, FROM_HERE);
//
//      // It is thread-safe to copy |base::WeakPtr|.
//      // Weak pointers may be passed safely between sequences, but must always be
//      // dereferenced and invalidated on the same SequencedTaskRunner otherwise
//      // checking the pointer would be racey.
//      return weak_this_;
//    }
//
//    // After constructing |weak_ptr_factory_|
//    // we immediately construct a WeakPtr
//    // in order to bind the WeakPtr object to its thread.
//    // When we need a WeakPtr, we copy construct this,
//    // which is safe to do from any
//    // thread according to weak_ptr.h (versus calling
//    // |weak_ptr_factory_.GetWeakPtr() which is not).
//    const ::base::WeakPtr<Listener> weak_this_
//      GUARDED_BY(fakeLockToSequence_);
//
//    /// \note It is not real lock, only annotated as lock.
//    /// It just calls callback on scope entry AND exit.
//    ::basis::FakeLockWithCheck<FakeLockRunType> fakeLockToSequence_{
//        ::base::BindRepeating(
//          &base::SequenceChecker::CalledOnValidSequence
//          , ::base::Unretained(&sequence_checker_)
//        )
//    };
//
//    // ...
//  };
template <
  typename FakeLockPolicyType
  , typename FakeLockCheckType
  , typename R
  , typename... Args
>
class SCOPED_LOCKABLE
  ScopedFakeLockWithCheck<
    FakeLockPolicyType
    , FakeLockCheckType
    , R(Args...)
  >
{
 public:
  using RunType = R(Args...);

  ScopedFakeLockWithCheck(
    const FakeLockWithCheck<RunType>& lock
    , ::base::Location from_here)
    EXCLUSIVE_LOCK_FUNCTION(lock)
    : lock_(lock)
    , from_here_(from_here)
  {
    if constexpr (!FakeLockCheckType::isExitScope)
    {
      if constexpr (FakeLockPolicyType::isDebugOnly
        && DCHECK_IS_ON())
      {
        DCHECK(lock_.Acquire())
          << from_here_.ToString();
      }
      // all except `isSkip` run check always
      if constexpr (!FakeLockPolicyType::isSkip)
      {
        CHECK(lock_.Acquire())
          << from_here_.ToString();
      }
    }
  }

  ~ScopedFakeLockWithCheck() UNLOCK_FUNCTION()
  {
    if constexpr (!FakeLockCheckType::isEnterScope)
    {
      if constexpr (FakeLockPolicyType::isDebugOnly
        && DCHECK_IS_ON())
      {
        DCHECK(lock_.Release())
          << from_here_.ToString();
      }
      // all except `isSkip` run check always
      if constexpr (!FakeLockPolicyType::isSkip)
      {
        CHECK(lock_.Release())
          << from_here_.ToString();
      }
    }
  }

 private:
  /// \note take care of reference limetime
  const FakeLockWithCheck<RunType>& lock_;

  ::base::Location from_here_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFakeLockWithCheck);
};

#define CREATE_FAKE_THREAD_GUARD(Name) \
  ::basis::FakeLockWithCheck<bool()> \
    Name { \
    ::basis::VerifyNothing::Repeatedly() \
  }

// Per-variable alternative to `GUARDED_BY`
// Documents that you must take care of thread safety somehow.
// That allows to notice (find & debug & fix) code
// that can be used from multiple threads.
#define GUARD_WITH_FAKE_LOCK(Name) \
  GUARDED_BY(Name); \
  CREATE_FAKE_THREAD_GUARD(Name)

#define CREATE_FAKE_THREAD_GUARD_WITH_CHECK(Name, Callback) \
  ::basis::FakeLockWithCheck<bool()> \
    Name { \
    Callback \
  }

// Prefer instead `GUARD_MEMBER_WITH_CHECK` to `SET_THREAD_GUARD_WITH_CHECK`
#define SET_THREAD_GUARD_WITH_CHECK(Name, Callback) \
  GUARDED_BY(Name); \
  CREATE_FAKE_THREAD_GUARD_WITH_CHECK(Name, Callback)

/// \notes requires to include:
/// #include <base/threading/thread_collision_warner.h>
/// #include <base/macros.h>
#define CREATE_RECURSIVE_THREAD_COLLISION_GUARD(Name, MutexName) \
  DFAKE_MUTEX(MutexName); \
  CREATE_FAKE_THREAD_GUARD_WITH_CHECK(Name \
    , ::base::BindRepeating( \
      [ \
      ]( \
        ::base::ThreadCollisionWarner& debug_collision_warner \
      ){ \
        DFAKE_SCOPED_RECURSIVE_LOCK(debug_collision_warner); \
        return true; \
      } \
      , REFERENCED(MutexName) \
    ) \
  )

// Sets thread collision warner to ensure that API is not called concurrently.
// API allowed to call from multiple threads, but not concurrently.
// Usually it means that var. can be used only by one thread at some
// moment of time (but different threads can use it overall).
// Imagine (as example) chain of `base::Promise` that uses same object
// on multiple threads, but all accesses are sequential
// (without thread collision).
#define SET_THREAD_COLLISION_GUARD(Name) \
  GUARDED_BY(Name); \
  CREATE_RECURSIVE_THREAD_COLLISION_GUARD( \
    Name, CGEN_UNIQUE_NAME(fake_mutex_))

/// \note Prefer instead RUN_ON(Name, m1, m2)
// Per-variable alternative to `RUN_ON`
// Documents that you must take care of thread safety somehow.
// That allows to notice (find & debug & fix) code
// that can be used from multiple threads.
#define THREAD_ANNOTATION_ATTRIBUTE__USE_CUSTOM_THREAD_GUARD(Name) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(Name))

// Guard name for member variable
#define MEMBER_GUARD(Name) \
  member_guard_##Name

// Guard name for function
#define FUNC_GUARD(Name) \
  func_guard_##Name

// Per-variable alternative to `DCHECK_RUN_ON`
// Documents that you must take care of thread safety somehow.
// That allows to notice (find & debug & fix) code
// that can be used from multiple threads.
#define FAKE_CUSTOM_THREAD_GUARD(Name, Policy, Scope) \
  ::basis::ScopedFakeLockWithCheck<\
    Policy\
    , Scope\
    , bool()\
  > \
    CGEN_UNIQUE_NAME(auto_lock_run_on_) \
      (Name, FROM_HERE)

// FakeLockCheckType::isWholeScope performs check on both scope enter and exit
#define DCHECK_THREAD_GUARD_SCOPE(Name) \
  FAKE_CUSTOM_THREAD_GUARD(Name, \
    ::basis::FakeLockPolicyDebugOnly, \
    ::basis::FakeLockCheckWholeScope)

// FakeLockCheckType::isEnterScope performs check only on scope enter
//
// EXAMPLE
//  /// \note `is_stream_valid_` may become invalid on scope exit,
//  /// so check it only on beginning of function scope,
//  /// but not at the end of function.
//  DCHECK_THREAD_GUARD_SCOPE_ENTER(MEMBER_GUARD(perConnectionStrand_));
#define DCHECK_THREAD_GUARD_SCOPE_ENTER(Name) \
  FAKE_CUSTOM_THREAD_GUARD(Name, \
    ::basis::FakeLockPolicyDebugOnly, \
    ::basis::FakeLockCheckEnterScope)

#define GUARD_MEMBER_DISALLOW_THREAD_COLLISION(Name) \
  SET_THREAD_COLLISION_GUARD(MEMBER_GUARD(Name))

// USAGE
//
//  ::basis::AnnotatedStrand<ExecutorType> perConnectionStrand_
//    GUARD_MEMBER_WITH_CHECK(
//      perConnectionStrand_
//      // 1. It safe to read value from any thread
//      // because its storage expected to be not modified.
//      // 2. On each access to strand check that stream valid
//      // otherwise `::boost::asio::post` may fail.
//      , ::base::BindRepeating(
//          [] \
//          (HttpChannel* self) -> bool {
//            DCHECK_THREAD_GUARD_SCOPE(self->guard_is_stream_valid_);
//            /// \note |perConnectionStrand_|
//            /// is valid as long as |stream_| valid
//            /// i.e. valid util |stream_| moved out
//            /// (it uses executor from stream).
//            return self->is_stream_valid_.load();
//          }
//          , ::base::Unretained(this)
//        )
//    );
#define GUARD_MEMBER_WITH_CHECK(Name, Callback) \
  SET_THREAD_GUARD_WITH_CHECK(MEMBER_GUARD(Name), Callback)

// FakeLockCheckType::isEnterScope performs check only on scope exit
#define DCHECK_THREAD_GUARD_SCOPE_EXIT(Name) \
  FAKE_CUSTOM_THREAD_GUARD(Name, \
    ::basis::FakeLockPolicyDebugOnly, \
    ::basis::FakeLockCheckExitScope)

/// \note avoid `DCHECK_CUSTOM_THREAD_GUARD_NOTHING` if you can
#define DCHECK_CUSTOM_THREAD_GUARD_NOTHING(Name) \
  FAKE_CUSTOM_THREAD_GUARD(Name, \
    ::basis::FakeLockPolicySkip, \
    ::basis::FakeLockCheckExitScope)

// Documents that variable allowed to be used from any thread
// and you MUST take care of thread-safety somehow.
//
// USE CASE EXAMPLES
//
// 1. Unmodified global variable can be used from any thread
// (if properly initialized).
// It safe to read value from any thread
// because its storage expected to be not modified,
// we just need to check storage validity.
// 2. Thread-safe type (like atomic).
#define THREAD_ANNOTATION_ATTRIBUTE__GUARDED_BY_ANY_THREAD(Name) \
  GUARDED_BY(Name)

/// \note Prefer instead `RUN_ON_ANY_THREAD_LOCKS_EXCLUDED`
// `RUN_ON_ANY_THREAD` can be used to force API users
// to use `DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD` on each call to some function because
// function is NOT thread-safe and must be avoided
// i.e. it makes API ugly intentionally
/// \note Alternatively you can combine it with multiple `mutexes`
/// like so: `RUN_ON(ANY_THREAD_GUARD(), m1, m2)`
// Documents that function allowed to be used from any thread
// and you MUST take care of thread-safety somehow.
//
// USAGE
//
//   void logFailure(const ErrorCode& ec, char const* what)
//     RUN_ON_ANY_THREAD(logFailure); // documents about thread-safety
//
//   // ...
//   CREATE_FAKE_THREAD_GUARD(logFailure);
//   // ...
//
//   {
//     DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(logFailure); // documents about thread-safety
//     logFailure(ec, "open");
//   }
//
// EXAMPLE
//
//   // can be used to acess registry on task runner
//   MUST_USE_RETURN_VALUE
//   ALWAYS_INLINE
//   ECS::Registry& registry() NO_EXCEPTION
//     /// \note force API users to use `DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD`
//     /// on each call to `registry()` because
//     /// function is NOT thread-safe and must be avoided
//     /// in preference to `operator*()` or `operator->()`
//     RUN_ON_ANY_THREAD(fn_registry) // documents about thread-safety
//   {
//     DCHECK_THREAD_GUARD_SCOPE(guard_registry_);
//
//     return registry_;
//   }
#define THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_ANY_THREAD(Name) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(Name))

// Use `RUN_ON_ANY_THREAD_LOCKS_EXCLUDED` if you want to
// document that function may be NOT thread-safe.
/// \note Alternatively you can combine it with multiple `mutexes`
/// like so: `LOCKS_EXCLUDED(Name, m1, m2)`
// Documents that function allowed to be used from any thread
// and you MUST take care of thread-safety somehow.
//
// USAGE
//
//   void logFailure(const ErrorCode& ec, char const* what)
//    RUN_ON_ANY_THREAD_LOCKS_EXCLUDED(logFailure) // documents about thread-safety
//  {
//    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(logFailure); // documents about thread-safety
//
//    // ...
//  }
//
//   // ...
//   CREATE_FAKE_THREAD_GUARD(logFailure);
//   // ...
//
//   {
//     logFailure(ec, "open");
//   }
#define THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_ANY_THREAD_LOCKS_EXCLUDED(Name) \
  LOCKS_EXCLUDED(Name)

// Allow to use code that can be used from any thread
// (in current scope only).
//
// Used for documentation purposes, so it is good idea
// to pass affected function names, variable names, etc.
//
// EXAMPLE
// class MySharedClass {
//   // ...
//   CREATE_FAKE_THREAD_GUARD(MySharedClassDestructor);
//   // ...
//   ~MySharedClass()
//   {
//     // documents that destructor called from any thread
//     DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(MySharedClassDestructor);
//   }
// };
#define DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(Name) \
  DCHECK_THREAD_GUARD_SCOPE_ENTER(FUNC_GUARD(Name))

#define DCHECK_MEMBER_GUARD(Name) \
  DCHECK_THREAD_GUARD_SCOPE_ENTER(MEMBER_GUARD(Name))

// Documents that class (or struct, etc.)
// member is not thread-bound
// i.e. can be used by multiple threads.
#define DCHECK_MEMBER_OF_UNKNOWN_THREAD(Name) \
  DCHECK_MEMBER_GUARD(Name)

// Documents that class (or struct, etc.)
// member is not thread-bound
// i.e. can be used by multiple threads.
//
// For example, it MAY be safe to read value from any thread
// if its storage expected to be not modified
// (if properly initialized)
#define GUARD_MEMBER_OF_UNKNOWN_THREAD(Name) \
  GUARD_WITH_FAKE_LOCK(MEMBER_GUARD(Name))

// Documents that class (or struct, etc.)
// `public:` member fuction is not thread-bound
// i.e. can be used by multiple threads.
#define GUARD_METHOD_ON_UNKNOWN_THREAD(Name) \
  THREAD_ANNOTATION_ATTRIBUTE__RUN_ON_ANY_THREAD_LOCKS_EXCLUDED(FUNC_GUARD(Name))

#define CREATE_METHOD_GUARD(Name) \
  CREATE_FAKE_THREAD_GUARD(FUNC_GUARD(Name))

#define CREATE_MEMBER_GUARD(Name) \
  CREATE_FAKE_THREAD_GUARD(MEMBER_GUARD(Name))

} // namespace basis
