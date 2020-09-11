#pragma once

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

#include <basis/unowned_ptr.hpp> // IWYU pragma: keep

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp> // IWYU pragma: keep
#include <boost/beast/core.hpp>

#include <basis/ECS/simulation_registry.hpp>

#include <vector>
#include <optional>

namespace ECS {

/// \note cause entt API is not thread-safe
/// we must wrap it to ensure thread-safety
// see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class AsioRegistry {
public:
  using StrandType
    = ::boost::asio::strand<::boost::asio::io_context::executor_type>;

  using IoContext
    = ::boost::asio::io_context;

  AsioRegistry(IoContext& ioc);

  ~AsioRegistry();

  /// \note works only if `Type` is `base::Optional<...>`
  /// because optional allows to re-create variable using same storage
  /// (i.e. using `placement new`)
  // If `Type` already exists it re-creates it using same storage
  // i.e. does NOT call `remove_if` and `vars_.push_back`.
  // Can be used to create `memory pool` where
  // unused data not freed instantly, but can be re-used again.
  template<typename Type, typename... Args>
  [[nodiscard]] /* don't ignore return value */
  Type & reset_or_create_var(
    const std::string debug_name
    , ECS::Entity tcp_entity_id
    , Args &&... args)
  {
    const bool useCache
      = registry_.has<Type>(tcp_entity_id);

    DVLOG(99)
      << (useCache
          ? ("using preallocated " + debug_name)
          : ("allocating new " + debug_name));

    Type& result
      = useCache
        /// \todo use get_or_emplace
        ? registry_.get<Type>(tcp_entity_id)
        : registry_.emplace<Type>(tcp_entity_id
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

  // Similar to |registry|, but without thread-safety checks
  // Example: can be used to acess registry on same thread
  // that created |AsioRegistry| i.e. may be useful to init registry
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ECS::Registry& registry_unsafe(
    const base::Location& from_here
    , base::StringPiece reason_why_unsafe
    , base::OnceClosure&& check_unsafe_allowed = base::DoNothing::Once()) const noexcept
  {
    ignore_result(from_here);
    ignore_result(reason_why_unsafe);
    base::rvalue_cast(check_unsafe_allowed).Run();
    return registry_;
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ECS::Registry& registry_unsafe(
    const base::Location& from_here
    , base::StringPiece reason_why_unsafe
    , base::OnceClosure&& check_unsafe_allowed = base::DoNothing::Once()) noexcept
  {
    ignore_result(from_here);
    ignore_result(reason_why_unsafe);
    base::rvalue_cast(check_unsafe_allowed).Run();
    return registry_;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ECS::Registry& registry() const noexcept
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread())
      << FROM_HERE.ToString();
    return registry_;
  }

  // can be used to acess registry on task runner
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ECS::Registry& registry() noexcept
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread())
      << FROM_HERE.ToString();
    return registry_;
  }

  NOT_THREAD_SAFE_FUNCTION()
  ALWAYS_INLINE
  bool running_in_this_thread() const NO_EXCEPTION
  {
    return asioRegistryStrand_.running_in_this_thread();
  }

  ALWAYS_INLINE
  NOT_THREAD_SAFE_FUNCTION()
  StrandType& strand()
  {
    return asioRegistryStrand_;
  }

  ALWAYS_INLINE
  NOT_THREAD_SAFE_FUNCTION()
  const StrandType& strand() const
  {
    return asioRegistryStrand_;
  }

  // strand copy equals same strand
  ALWAYS_INLINE
  NOT_THREAD_SAFE_FUNCTION()
  const StrandType copy_strand() const
  {
    return asioRegistryStrand_;
  }

  // strand copy equals same strand
  ALWAYS_INLINE
  NOT_THREAD_SAFE_FUNCTION()
  StrandType copy_strand()
  {
    return asioRegistryStrand_;
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
    return registry();
  }

  constexpr ECS::Registry& operator*()
  {
    return registry();
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
    return &registry();
  }

  constexpr ECS::Registry* operator->()
  {
    return &registry();
  }

private:
  SEQUENCE_CHECKER(sequence_checker_);

  // modification of |asioRegistry_| guarded by |asioRegistryStrand_|
  /// \note do not destruct |Listener| while |asioRegistryStrand_|
  /// has scheduled or execting tasks
  NOT_THREAD_SAFE_LIFETIME()
  StrandType asioRegistryStrand_;

  ECS::SimulationRegistry asioRegistry_{}
  LIVES_ON(asioRegistryStrand_);

  // base::WeakPtr can be used to ensure that any callback bound
  // to an object is canceled when that object is destroyed
  // (guarantees that |this| will not be used-after-free).
  base::WeakPtrFactory<
      AsioRegistry
    > weak_this_factory_;

  // Registry stores entities and arranges pools of components
  /// \note entt API is not thread-safe
  ECS::Registry registry_{};

  DISALLOW_COPY_AND_ASSIGN(AsioRegistry);
};

} // namespace ECS
