#pragma once

#include <basis/ECS/ecs.h>

#include <base/logging.h>

namespace ECS {

/// \note returns ECS::NULL_ENTITY if can not find
/// entity with desired components
/// \note will return only one entity that matches filter
template<
  // Desired components.
  class... Include
  // `Exclude...` can be used to skip entity with specific tags.
  // For example, entity marked as `NeedToDestroyTag`
  // (i.e. currently deallocating entity) may be ignored.
  , class... Exclude
>
static ECS::Entity findEntity(
  ECS::Registry& registry
  , ECS::include_t<Include...> = {}
  , ECS::exclude_t<Exclude...> = {})
{
  auto registry_group
    = registry.view<Include...>(
        entt::exclude<
          Exclude...
        >
      );

  if(!registry_group.empty()) // if found entity with desired components
  {
    for(const ECS::Entity& entityId : registry_group)
    {
      DCHECK(registry.valid(entityId));
      return entityId;
    }
  }

  return ECS::NULL_ENTITY;
}

} // namespace ECS
