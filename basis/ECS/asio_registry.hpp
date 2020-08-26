#pragma once

#include <base/timer/timer.h>
#include <base/time/time.h>
#include <base/bind.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/optional.h>
#include <base/files/file_path.h>
#include <base/trace_event/trace_event.h>
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

  AsioRegistry(util::UnownedPtr<IoContext>&& ioc);

  ~AsioRegistry();

  // can be used to acess registry on same thread that created |AsioRegistry|,
  // may be useful to init registry
  ECS::Registry& ref_registry_unsafe(const base::Location& from_here) noexcept;

  // can be used to acess registry on task runner
  ECS::Registry& ref_registry(const base::Location& from_here) noexcept;

  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  ECS::Entity create(
    const base::Location& from_here)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    return ref_registry(from_here).create();
  }

  template<typename... Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) get(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (ref_registry(from_here).get<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(ref_registry(from_here).get<Component>(entity)...);
    }
  }

  template<typename... Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) try_get(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (ref_registry(from_here).try_get<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(ref_registry(from_here).try_get<Component>(entity)...);
    }
  }

  template<typename Component, typename... Args>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) get_or_emplace(
    const base::Location& from_here
    , const ECS::Entity entity
    , Args &&... args)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));

    return ref_registry(from_here).get_or_assign<Component>(entity, std::forward<Args>(args)...);
  }

  template<typename... Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) clear(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (ref_registry(from_here).clear<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(ref_registry(from_here).clear<Component>(entity)...);
    }
  }

  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool has_components(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    return !ref_registry(from_here).orphan(entity);
  }

  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool valid(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    return ref_registry(from_here).valid(entity);
  }

  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool has(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    return ref_registry(from_here).has<Component>(entity);
  }

  template<typename... Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool any(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    return (ref_registry(from_here).any<Component>(entity) || ...);
  }

  template<typename Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void destroy(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    ref_registry(from_here).destroy<Component>(entity);
  }

  template<typename It>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void destroy(
    const base::Location& from_here
    , It first
    , It last)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    while(first != last) { ref_registry(from_here).destroy(*(first++)); }
  }

  template<typename Component, typename... Args>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void emplace(
    const base::Location& from_here
    , ECS::Entity entity
    , Args &&... args)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    ref_registry(from_here).emplace<Component>(entity, std::forward<Args>(args)...);
  }

  /// \note Prefer this function anyway because it has slightly better performance.
  /// Equivalent to the following snippet (pseudocode):
  /// auto &component = registry.has<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.emplace<Component>(entity, args...);
  template<typename Component, typename... Args>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void emplace_or_replace(
    const base::Location& from_here
    , ECS::Entity entity
    , Args &&... args)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    ref_registry(from_here).assign_or_replace<Component>(entity, std::forward<Args>(args)...);
  }

  template<typename... Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void remove(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    (ref_registry(from_here).remove<Component>()(entity), ...);
  }

  template<typename... Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void remove_if_exists(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(asioRegistryStrand_.running_in_this_thread());
    DCHECK(ref_registry(from_here).valid(entity));
    (ref_registry(from_here).remove_if_exists<Component>()(entity), ...);
  }

  NOT_THREAD_SAFE_FUNCTION()
  inline /* `inline` to eleminate function call overhead */
  bool running_in_this_thread()
  {
    return asioRegistryStrand_.running_in_this_thread();
  }

  NOT_THREAD_SAFE_FUNCTION()
  StrandType& ref_strand(const base::Location& from_here)
  {
    ignore_result(from_here);
    return asioRegistryStrand_;
  }

  // strand copy equals same strand
  NOT_THREAD_SAFE_FUNCTION()
  StrandType copy_strand(const base::Location& from_here)
  {
    ignore_result(from_here);
    return asioRegistryStrand_;
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
