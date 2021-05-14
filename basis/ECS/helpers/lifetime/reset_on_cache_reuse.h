#pragma once

#include <basis/ECS/ecs.h>

namespace ECS {

// Resets components for entity retrieved from cache.
//
// EXAMPLE
//
// // removes entity from hierarchy
// resetOnCacheReuse<ConnectionTag>(entityId);
//
template<typename T>
void resetOnCacheReuse(
  ECS::Registry& registry
  , ECS::Entity entityId);

} // namespace ECS
