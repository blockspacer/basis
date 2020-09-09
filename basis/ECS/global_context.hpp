/**
 * Based on code from entt v3.4.0
 * See https://github.com/skypjack/entt/blob/v3.4.0/src/entt/entity/registry.hpp
 * Made modifications to fully access `std::vector<variable_data> vars{};`
 * and perform thread-safety validations.
**/

#pragma once

#include "basis/ECS/ecs.hpp"
#include "basis/ECS/unsafe_context.hpp"

#include <entt/core/type_traits.hpp>

#include <base/macros.h>
#include <base/logging.h>
#include <base/location.h>
#include <base/sequenced_task_runner.h>
#include <base/threading/thread_checker.h>
#include <base/memory/weak_ptr.h>
#include <base/memory/ref_counted.h>
#include <base/memory/scoped_refptr.h>
#include <base/threading/thread_collision_warner.h>

#include <vector>
#include <string>
#include <atomic>

namespace base {

class SingleThreadTaskRunner;

template <typename T>
struct DefaultSingletonTraits;

} // namespace base

namespace ECS {

/// \todo Rename `GlobalContext` to `TheadSafeGlobalContext` and use `mutex`
#if 0
/// \note Prefer `TheadSafeGlobalContext` to `GlobalContext`
///
// Context that can be used as global singleton.
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
///
/// \note Prefer to use |GlobalContext|
/// instead of custom singletons.
///
/// MOTIVATION
///
/// * Plugin system not compatible with `singleton` pattern,
/// so pass global context as not-global object during plugin creation.
///
/// \note API is not thread-safe,
/// so set vars only at `preload` state
/// and unset only at `stop` state.
/// Using that approach you will be able
/// to use variable from multiple threads cause
/// during `running` state:
/// 1) var must be created
/// 2) var assumed to be not be changed.
///
// |GlobalContext| is not thread-safe,
// so modify it only from one sequence.
// Use |lockModification| to guarantee
// thread-safety for read-only operations.
class GlobalContext
{
public:
  GlobalContext();

  /// \todo remove in favor of `SequenceLocalContext`
  /// Rename `GlobalContext` to `TheadSafeGlobalContext` and use `mutex`
  // prohibit entity creation/destruction for thread-safety purposes
  // i.e. fill |GlobalContext| before app started, then call `lock()`.
  // Call `unlock()` only during app termination.
  void lockModification();

  /// \todo remove in favor of `SequenceLocalContext`
  /// Rename `GlobalContext` to `TheadSafeGlobalContext` and use `mutex`
  // allow entity creation/destruction for thread-safety purposes
  // i.e. fill |GlobalContext| before app started, then call `lock()`.
  // Call `unlock()` only during app termination.
  void unlockModification();

  bool isLockedModification() const
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_collision_warner_);

    return locked_.load();
  }

  /// \note Every call incurs some overhead
  /// to check whether the object has already been initialized.
  /// You may wish to cache the result of get(); it will not change.
  static GlobalContext* GetInstance();

  // usually context is locked
  // after app creation and before app termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Component& ctx_locked(
    const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    /// \note no thread-safety checks
    DCHECK(locked_.load())
      << "Unable to use LOCKED global context from "
      << from_here.ToString();

    DCHECK(context_.try_ctx_var<Component>())
      << "failed GlobalContext::ctx from "
      << from_here.ToString();

    return context_.ctx_var<Component>();
  }

  // usually context is locked
  // after app creation and before app termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool try_ctx_locked(
    const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    /// \note no thread-safety checks
    DCHECK(locked_.load())
      << "Unable to use LOCKED global context from "
      << from_here.ToString();

#if DCHECK_IS_ON()
    if(!context_.try_ctx_var<Component>())
    {
      // just some extra logging for debug purposes
      DVLOG(9)
        << "result GlobalContext::try_ctx_var is false from "
        << from_here.ToString();
    }
#endif // DCHECK_IS_ON()

    return context_.try_ctx_var<Component>();
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Component& ctx_unlocked(
    const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(!locked_.load())
      << "Unable to use LOCKED global context from "
      << from_here.ToString();

    DCHECK(context_.try_ctx_var<Component>())
      << "failed GlobalContext::ctx from "
      << from_here.ToString();

    return context_.ctx_var<Component>();
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool try_ctx_unlocked(
    const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(!locked_.load())
      << "Unable to use LOCKED global context from "
      << from_here.ToString();

#if DCHECK_IS_ON()
    if(!context_.try_ctx_var<Component>())
    {
      // just some extra logging for debug purposes
      DVLOG(9)
        << "result GlobalContext::try_ctx_var is false from "
        << from_here.ToString();
    }
#endif // DCHECK_IS_ON()

    return context_.try_ctx_var<Component>();
  }

  template<typename Type, typename... Args>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Type& set_once(const base::Location& from_here
    , const std::string& debug_name
    , Args&&... args)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(!locked_.load())
      << "Unable to use NOT LOCKED global context from "
      << from_here.ToString();

    DVLOG(9)
      << "called GlobalContext::set from "
      << from_here.ToString()
      << " added to global context: "
      << debug_name;

    /// \note can be set only once
    DCHECK(!context_.try_ctx_var<Type>());
    return context_.set_var<Type>(
      debug_name
      , std::forward<Args>(args)...);
  }

  template<typename Type>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void unset(const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(!locked_.load())
      << "Unable to use NOT LOCKED global context from "
      << from_here.ToString();

    DVLOG(9)
      << "called GlobalContext::unset from "
      << from_here.ToString();

    DCHECK(context_.try_ctx_var<Type>());
    return context_.unset_var<Type>();
  }

  template<typename Type>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void try_unset(const base::Location& from_here)
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(!locked_.load())
      << "Unable to use NOT LOCKED global context from "
      << from_here.ToString();

    DVLOG(9)
      << "called GlobalContext::unset from "
      << from_here.ToString();

    return context_.unset_var<Type>();
  }

private:
  ~GlobalContext();

  size_t size() const
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    if(!isLockedModification()) {
      DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
    }
    return context_.size();
  }

  bool empty() const
  {
    DFAKE_SCOPED_LOCK(debug_collision_warner_);

    if(!isLockedModification()) {
      DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
    }
    return context_.empty();
  }

private:
  friend struct base::DefaultSingletonTraits<GlobalContext>;

  // Usually you want to unlock during app creation and termination
  // to guarantee thread-safety.
  std::atomic<bool> locked_ = true;

  UnsafeTypeContext context_{};

  /// \note Unlike `main_thread_checker_` collision warner used
  /// BOTH buring modification and reading
  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_collision_warner_);

  /// \note `main_thread_checker_` used buring modification,
  /// but not during reading
  THREAD_CHECKER(main_thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(GlobalContext);
};
#endif // 0

} // namespace ECS
