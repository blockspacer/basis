﻿#include "basis/ECS/helpers/lifetime/populate_delayed_construction_components.h" // IWYU pragma: associated
#include <basis/ECS/tags.h>

namespace ECS {

void populateDelayedConstructionComponents(
  ECS::Registry& registry
  , ECS::Entity entityId)
{
  // mark entity as not fully created
  registry.template emplace_or_replace<
      ECS::DelayedConstruction
    >(entityId);

  // mark entity as not fully created
  registry.template remove_if_exists<
      ECS::DelayedConstructionJustDone
    >(entityId);
}

} // namespace ECS
