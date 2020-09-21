#pragma once

#include "basis/ECS/ecs.hpp"

#include "basis/lock_with_check.hpp"
#include "basis/unowned_ptr.hpp" // IWYU pragma: keep
#include "basis/lock_with_check.hpp" // IWYU pragma: keep

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

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp> // IWYU pragma: keep
#include <boost/beast/core.hpp>

#include <basis/ECS/simulation_registry.hpp>

#include <vector>

namespace ECS {

/// \note cause entt API is not thread-safe
/// we must wrap it to ensure thread-safety
// see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class LOCKABLE AsioRegistry {
public:
  using ExecutorType
    = ::boost::asio::io_context::executor_type;

  using StrandType
    = ::boost::asio::strand<ExecutorType>;

  using IoContext
    = ::boost::asio::io_context;

  AsioRegistry(IoContext& ioc);

  ~AsioRegistry();

  SET_WEAK_SELF(AsioRegistry)

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
  [[nodiscard]] /* don't ignore return value */
  base::Optional<typename Type::value_type> & reset_or_create_var(
    const std::string debug_name
    , ECS::Entity tcp_entity_id
    , Args &&... args)
    RUN_ON(&strand)
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(strand));
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(registry_));

    DCHECK(strand->running_in_this_thread());

    const bool useCache
      = registry_.has<base::Optional<typename Type::value_type>>(tcp_entity_id);

    DVLOG(99)
      << (useCache
          ? ("using preallocated " + debug_name)
          : ("allocating new " + debug_name));

    base::Optional<typename Type::value_type>& result
      = useCache
        /// \todo use get_or_emplace
        ? registry_.get<base::Optional<typename Type::value_type>>(tcp_entity_id)
        : registry_.emplace<base::Optional<typename Type::value_type>>(tcp_entity_id
            , base::in_place
            , std::forward<Args>(args)...);

    // If the value already exists it is overwritten
    if(useCache)
    {
      DCHECK(result);
      /// \note we do not call `emplace` for optimization purposes
      /// ( because `emplace` uses `erase` and `instances.push_back`)
      /// i.e. use `base::Optional<...>` that uses placement new
      result.emplace(std::forward<Args>(args)...);
    }

    return result;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ECS::Registry& registry() const NO_EXCEPTION
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(registry_));

    DCHECK_RUN_ON_ANY_THREAD_SCOPE(FUNC_GUARD(registry));

    return registry_;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ECS::Registry& registry() NO_EXCEPTION
    /// \note force API users to use `DCHECK_RUN_ON_ANY_THREAD_SCOPE`
    /// on each call to `registry()` because
    /// function is NOT thread-safe and must be avoided
    /// in preference to `operator*()` or `operator->()`
    RUN_ON_ANY_THREAD(FUNC_GUARD(registry))
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(registry_));

    return registry_;
  }

  ALWAYS_INLINE
  bool running_in_this_thread() const NO_EXCEPTION
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(strand));

    DCHECK_RUN_ON_ANY_THREAD_SCOPE(FUNC_GUARD(running_in_this_thread));

    return strand->running_in_this_thread();
  }

  ALWAYS_INLINE
  StrandType& asioStrand()
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(strand));

    DCHECK_RUN_ON_ANY_THREAD_SCOPE(FUNC_GUARD(asioStrand));

    return strand.data;
  }

  ALWAYS_INLINE
  const StrandType& asioStrand() const
  {
    DCHECK_THREAD_GUARD_SCOPE(MEMBER_GUARD(strand));

    DCHECK_RUN_ON_ANY_THREAD_SCOPE(FUNC_GUARD(asioStrand));

    return strand.data;
  }

  // Shortcut for `.registry`
  //
  // BEFORE
  // DCHECK(obj.registry().empty());
  //
  // AFTER
  // DCHECK((*obj).empty());
  constexpr const ECS::Registry& operator*() const
  {
    /// \note we assume that purpose of
    /// calling `operator*` is to change registry,
    /// so we need to validate thread-safety
    DCHECK(running_in_this_thread());

    return registry_;
  }

  constexpr ECS::Registry& operator*()
  {
    /// \note we assume that purpose of
    /// calling `operator*` is to change registry,
    /// so we need to validate thread-safety
    DCHECK(running_in_this_thread());

    return registry_;
  }

  // Shortcut for `.registry`
  //
  // BEFORE
  // DCHECK(obj.registry().empty());
  //
  // AFTER
  // DCHECK(obj->empty());
  constexpr const ECS::Registry* operator->() const
  {
    /// \note we assume that purpose of
    /// calling `operator->` is to change registry,
    /// so we need to validate thread-safety
    DCHECK(running_in_this_thread());

    return &registry_;
  }

  constexpr ECS::Registry* operator->()
  {
    /// \note we assume that purpose of
    /// calling `operator->` is to change registry,
    /// so we need to validate thread-safety
    DCHECK(running_in_this_thread());

    return &registry_;
  }

public:
  // modification of |asioRegistry_| guarded by |strand|
  /// \note do not destruct |Listener| while |strand|
  /// has scheduled or execting tasks
  basis::AnnotatedStrand<ExecutorType> strand
    SET_STORAGE_THREAD_GUARD(MEMBER_GUARD(strand));

  /// \note `registry(...)` is not thread-safe
  CREATE_CUSTOM_THREAD_GUARD(FUNC_GUARD(registry));

private:
  SEQUENCE_CHECKER(sequence_checker_);

  // Registry stores entities and arranges pools of components
  /// \note entt API is not thread-safe
  ECS::Registry registry_
    SET_STORAGE_THREAD_GUARD(MEMBER_GUARD(registry_));

  /// \note `running_in_this_thread(...)` is not thread-safe
  CREATE_CUSTOM_THREAD_GUARD(FUNC_GUARD(running_in_this_thread));

  /// \note `asioStrand(...)` is not thread-safe
  CREATE_CUSTOM_THREAD_GUARD(FUNC_GUARD(asioStrand));

  SET_WEAK_POINTERS(AsioRegistry);

  DISALLOW_COPY_AND_ASSIGN(AsioRegistry);
};

} // namespace ECS
