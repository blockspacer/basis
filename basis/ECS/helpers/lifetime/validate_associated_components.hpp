#pragma once

#include <basis/ECS/ecs.hpp>

namespace ECS {

// Check components (existence and stored values) associated with type.
//
// EXAMPLE
//
// // crash if entity does not exist in hierarchy
// CHECK(validateAssociatedComponents<TreeElement>(entityId);
//
template<typename T>
bool validateAssociatedComponents(
  ECS::Registry& registry
  , ECS::Entity entityId);

} // namespace ECS
