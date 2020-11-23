#pragma once

#if 0
#include "basis/ECS/ecs.hpp"

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

#include <vector>
#include <optional>

namespace ECS {

/// \note cause entt API is not thread-safe
/// we must wrap it to ensure thread-safety
// see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class SimulationRegistry {
public:
  SimulationRegistry();

  void set_task_runner(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    noexcept;

  ~SimulationRegistry();

  // can be used to acess registry on same thread that created |SimulationRegistry|,
  // may be useful to init registry
  ECS::Registry& registry_unsafe(const base::Location& from_here) noexcept;

  // can be used to acess registry on task runner
  ECS::Registry& registry() noexcept;

  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  ECS::Entity create(const base::Location& from_here)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    return registry().create();
  }

  template<typename... Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) get(const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (registry().get<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(registry().get<Component>(entity)...);
    }
  }

  template<typename... Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) try_get(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (registry().try_get<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(registry().try_get<Component>(entity)...);
    }
  }

  template<typename Component, typename... Args>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) get_or_emplace(
    const base::Location& from_here
    , const ECS::Entity entity
    , Args &&... args)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));

    return registry().get_or_assign<Component>(entity, std::forward<Args>(args)...);
  }

  template<typename... Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  decltype(auto) clear(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));

    if constexpr(sizeof...(Component) == 1) {
        return (registry().clear<Component>(entity), ...);
    } else {
        return std::forward_as_tuple(registry().clear<Component>(entity)...);
    }
  }

  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool has_components(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    return !registry().orphan(entity);
  }

  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool valid(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    return registry().valid(entity);
  }

  template<typename Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool has(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    return registry().has<Component>(entity);
  }

  template<typename... Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool any(
    const base::Location& from_here
    , const ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    return (registry().any<Component>(entity) || ...);
  }

  template<typename Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void destroy(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    registry().destroy<Component>(entity);
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
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
      while(first != last) { registry().destroy(*(first++)); }
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
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    registry().assign<Component>(entity, std::forward<Args>(args)...);
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
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    registry().assign_or_replace<Component>(entity, std::forward<Args>(args)...);
  }

  template<typename... Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void remove(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    (registry().remove<Component>()(entity), ...);
  }

  template<typename... Component>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void remove_if_exists(
    const base::Location& from_here
    , ECS::Entity entity)
  {
    DCHECK(task_runner_);
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    DCHECK(registry().valid(entity));
    (registry().remove_if_exists<Component>()(entity), ...);
  }

  scoped_refptr<
      base::SequencedTaskRunner
    > task_runner()
  {
    DCHECK(task_runner_);
    return task_runner_;
  }

private:
  SEQUENCE_CHECKER(sequence_checker_);

  scoped_refptr<
      base::SequencedTaskRunner
    > task_runner_;

  // base::WeakPtr can be used to ensure that any callback bound
  // to an object is canceled when that object is destroyed
  // (guarantees that |this| will not be used-after-free).
  base::WeakPtrFactory<
      SimulationRegistry
    > weak_this_factory_;

  // Registry stores entities and arranges pools of components
  /// \note entt API is not thread-safe
  ECS::Registry registry_{};

  DISALLOW_COPY_AND_ASSIGN(SimulationRegistry);
};

} // namespace ECS
#endif // 0
