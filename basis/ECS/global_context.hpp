/**
 * Based on code from entt v3.4.0
 * See https://github.com/skypjack/entt/blob/v3.4.0/src/entt/entity/registry.hpp
 * Made modifications to fully access `std::vector<variable_data> vars{};`
 * and perform thread-safety validations.
**/

#if 0
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

/// \note Prefer `TheadSafeGlobalContext` to `GlobalContext`
///
// Context that can be used as global singleton.
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
///
/// \note Prefer to use |UnsafeGlobalContext|
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
// |UnsafeGlobalContext| is not thread-safe,
// so modify it only from one sequence.
// Use |lockModification| to guarantee
// thread-safety for read-only operations.
class UnsafeGlobalContext
{
public:
  /// \note Every call incurs some overhead
  /// to check whether the object has already been initialized.
  /// You may wish to cache the result of get(); it will not change.
  static UnsafeGlobalContext* GetInstance();

  // usually context is locked
  // after app creation and before app termination
  MUST_USE_RESULT
  ALWAYS_INLINE
  UnsafeTypeContext& context()
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    return context_;
  }

private:
  // private due to singleton
  UnsafeGlobalContext();

  // private due to singleton
  ~UnsafeGlobalContext();

private:
  friend struct ::base::DefaultSingletonTraits<UnsafeGlobalContext>;

  UnsafeTypeContext context_{};

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(UnsafeGlobalContext);
};

} // namespace ECS
#endif // 0
