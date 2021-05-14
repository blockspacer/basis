#pragma once

#include <basis/ECS/ecs.h>

namespace ECS {

/// \note Usually you do not need to remove all components
/// before entity destruction.
//
// Remove components associated with type.
//
// EXAMPLE
//
// // removes entity from hierarchy
// removeAssociatedComponents<TreeElement>(entityId);
//
template<typename T>
void removeAssociatedComponents(
  ECS::Registry& registry
  , ECS::Entity entityId);

} // namespace ECS
