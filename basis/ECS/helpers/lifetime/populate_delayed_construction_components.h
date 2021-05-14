#pragma once

#include <basis/ECS/ecs.h>

namespace ECS {

// We want to add custom components to entity from plugins.
// So upon construction, entity must have `ECS::DelayedConstruction` component.
// We assume that `entity` will be constructed within 1 tick,
// then delete `ECS::DelayedConstruction` component
// \note Do not forget to skip entity updates
// if it has `ECS::DelayedConstruction` component.
// \note Do not forget to properly free entity during termination
// if it has `ECS::DelayedConstruction` component
// (i.e. app closes while some entity still not constructed).
// \note Make sure that not fully created entities are properly freed
// (usually that means that they must have some relationship component
// like `FirstChildComponent`, `ChildSiblings` etc.
// that will allow them to be freed upon parent entity destruction).
void populateDelayedConstructionComponents(
  ECS::Registry& registry
  , ECS::Entity entityId);

} // namespace ECS
