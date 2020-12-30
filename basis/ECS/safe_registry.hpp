#pragma once

#include "basis/ECS/ecs.hpp"

#include "basis/unowned_ptr.hpp" // IWYU pragma: keep
#include "basis/checks_and_guard_annotations.hpp" // IWYU pragma: keep

#include <base/timer/timer.h>
#include <base/time/time.h>
#include <base/bind.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/optional.h>
#include <base/rvalue_cast.h>
#include <base/files/file_path.h>
#include <base/trace_event/trace_event.h>
#include <base/strings/string_piece.h>
#include <base/bind_helpers.h>
#include <base/synchronization/waitable_event.h>
#include <base/observer_list_threadsafe.h>
#include <base/task/thread_pool/thread_pool.h>

#include <vector>

namespace ECS {

// `reset_or_create_component` works only with `base::Optional` for now
namespace safe_registry_internal {
template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<base::Optional<T>> : std::true_type {};
} // namespace safe_registry_internal

// Safe registry is entt::registry
// bound to task runner (for thread-safety reasons).
// see for details:
// https://github.com/skypjack/entt/wiki
/// \note cause entt API is not thread-safe
/// we must wrap it to ensure thread-safety
class LOCKABLE SafeRegistry {
public:
  SafeRegistry();

  ~SafeRegistry();

  using TaskRunnerType
    = scoped_refptr<::base::SequencedTaskRunner>;

  SET_WEAK_SELF(SafeRegistry)

  /// \note works only if `Type` is `base::Optional<...>`
  /// because optional allows to re-create variable using same storage
  /// (i.e. using `placement new`)
  // If `Type` already exists it re-creates it using same storage
  // i.e. does NOT call `remove_if` and `vars_.push_back`.
  // Can be used to create `memory pool` where
  // unused data not freed instantly, but can be re-used again.
  /// \note `base::Optional<typename Type::value_type>` and `Type` must be same,
  /// we just want to force user to use `base::Optional`
  template<typename Type, typename... Args>
  MUST_USE_RETURN_VALUE
  ::base::Optional<typename Type::value_type>& reset_or_create_component(
    const std::string debug_name
    , ECS::Entity tcp_entity_id
    , Args &&... args)
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(registry_);

    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    static_assert(ECS::safe_registry_internal::is_optional<Type>::value,
                  "Use reset_or_create_component only if type is base::Optional.");

    const bool useCache
      = registry_.has<::base::Optional<typename Type::value_type>>(tcp_entity_id);

    DVLOG(99)
      << (useCache
          ? ("using preallocated " + debug_name)
          : ("allocating new " + debug_name));

    ::base::Optional<typename Type::value_type>& result
      = useCache
        /// \todo use get_or_emplace
        ? registry_.get<::base::Optional<typename Type::value_type>>(tcp_entity_id)
        : registry_.emplace<::base::Optional<typename Type::value_type>>(tcp_entity_id
            , ::base::in_place
            , FORWARD(args)...);

    // If the value already exists it is overwritten
    if(useCache)
    {
      DCHECK(result);
      /// \note we do not call `emplace` for optimization purposes
      /// ( because `emplace` uses `erase` and `instances.push_back`)
      /// i.e. use `base::Optional<...>` that uses placement new
      result.emplace(FORWARD(args)...);
    }

    return result;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ECS::Registry& registry() const NO_EXCEPTION
    GUARD_METHOD_ON_UNKNOWN_THREAD(registryUnsafe)
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(registry_);

    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(registryUnsafe);

    return registry_;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ECS::Registry& registryUnsafe() NO_EXCEPTION
    GUARD_METHOD_ON_UNKNOWN_THREAD(registryUnsafe)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(registryUnsafe);

    DCHECK_MEMBER_OF_UNKNOWN_THREAD(registry_);

    return registry_;
  }

  ALWAYS_INLINE
  bool RunsTasksInCurrentSequence() const NO_EXCEPTION
    GUARD_METHOD_ON_UNKNOWN_THREAD(RunsTasksInCurrentSequence)
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(RunsTasksInCurrentSequence);

    return taskRunner_->RunsTasksInCurrentSequence();
  }

  ALWAYS_INLINE
  TaskRunnerType& taskRunner()
    GUARD_METHOD_ON_UNKNOWN_THREAD(taskRunner)
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(taskRunner);

    return taskRunner_;
  }

  ALWAYS_INLINE
  const TaskRunnerType& taskRunner() const
    GUARD_METHOD_ON_UNKNOWN_THREAD(taskRunner)
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(taskRunner);

    return taskRunner_;
  }

  // MOTIVATION
  //
  // Allows to convert `SafeRegistry` to `ECS::Registry&` implicitly
  // i.e. function that accepts `ECS::Registry&` will accept `SafeRegistry`.
  //
  // USAGE
  //
  // ECS::Registry& ecs_reg = static_cast<ECS::Registry&>(safe_reg);
  operator ECS::Registry&() NO_EXCEPTION
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    /// \note we assume that purpose of
    /// calling `operator*` is to change registry,
    /// so we need to validate thread-safety
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    return registry_;
  }

  // Shortcut for `.registry`
  //
  // BEFORE
  // DCHECK(obj.registry().empty());
  //
  // AFTER
  // DCHECK((*obj).empty());
  const ECS::Registry& operator*() const NO_EXCEPTION
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    /// \note we assume that purpose of
    /// calling `operator*` is to change registry,
    /// so we need to validate thread-safety
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    return registry_;
  }

  ECS::Registry& operator*() NO_EXCEPTION
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    /// \note we assume that purpose of
    /// calling `operator*` is to change registry,
    /// so we need to validate thread-safety
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    return registry_;
  }

  // Shortcut for `.registry`
  //
  // BEFORE
  // DCHECK(obj.registry().empty());
  //
  // AFTER
  // DCHECK(obj->empty());
  const ECS::Registry* operator->() const
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    /// \note we assume that purpose of
    /// calling `operator->` is to change registry,
    /// so we need to validate thread-safety
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    return &registry_;
  }

  ECS::Registry* operator->()
    PUBLIC_METHOD_RUN_ON(taskRunner_.get())
  {
    DCHECK_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

    /// \note we assume that purpose of
    /// calling `operator->` is to change registry,
    /// so we need to validate thread-safety
    DCHECK_RUN_ON_SEQUENCED_RUNNER(taskRunner_.get());

    return &registry_;
  }

public:
  /// \note `registry(...)` is not thread-safe
  CREATE_METHOD_GUARD(registryUnsafe);
  CREATE_METHOD_GUARD(RunsTasksInCurrentSequence);
  CREATE_METHOD_GUARD(taskRunner);

private:
  SEQUENCE_CHECKER(sequence_checker_);

  // modification of |SafeRegistry_| guarded by |taskRunner_|
  /// \note do not destruct |Listener| while |taskRunner_|
  /// has scheduled or executing tasks
  TaskRunnerType taskRunner_
    GUARD_MEMBER_OF_UNKNOWN_THREAD(taskRunner_);

  // Registry stores entities and arranges pools of components
  /// \note entt API is not thread-safe
  ECS::Registry registry_
    GUARD_MEMBER_OF_UNKNOWN_THREAD(registry_);

  SET_WEAK_POINTERS(SafeRegistry);

  DISALLOW_COPY_AND_ASSIGN(SafeRegistry);
};

// Helper class used by DCHECK_RUN_ON
class SCOPED_LOCKABLE NetRegistryScope {
 public:
  explicit NetRegistryScope(
    const ECS::SafeRegistry* thread_like_object)
      EXCLUSIVE_LOCK_FUNCTION(thread_like_object) {}

  NetRegistryScope(
    const NetRegistryScope&) = delete;

  NetRegistryScope& operator=(
    const NetRegistryScope&) = delete;

  ~NetRegistryScope() UNLOCK_FUNCTION() {}

  static bool RunsTasksInCurrentSequence(
    const ECS::SafeRegistry* thread_like_object)
  {
    return thread_like_object->RunsTasksInCurrentSequence();
  }
};

// Type of `x` is `SafeRegistry*`
//
// USAGE
//
// SafeRegistry registry_
//   // It safe to read value from any thread because its storage
//   // expected to be not modified (if properly initialized)
//   SET_CUSTOM_THREAD_GUARD(registry_);
// // ...
// DCHECK_THREAD_GUARD_SCOPE(registry_);
// DCHECK_RUN_ON_REGISTRY(&registry_);
#define DCHECK_RUN_ON_REGISTRY(x)                                              \
  ECS::NetRegistryScope internal_registry_scope(x); \
  DCHECK((x)); \
  DCHECK((x)->RunsTasksInCurrentSequence())

} // namespace ECS
