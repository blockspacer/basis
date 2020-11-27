#pragma once

#include <basis/ECS/ecs.hpp>

namespace ECS {

// populate (create and set initial values) components associated with type,
// usually required by newly created entity.
//
/// \note Entity may have some components if it was retrieved from cache.
//
template<typename T>
void populateAssociatedComponents(
  ECS::Registry& registry
  , ECS::Entity entityId);

} // namespace ECS
