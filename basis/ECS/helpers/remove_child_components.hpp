#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

// remove all components associated with `child`
template <
  typename TagType // unique type tag for all children
>
void removeChildComponents(
  ECS::Registry& registry
  , ECS::Entity childId)
{
  using ChildrenComponent = ChildLinkedList<TagType>;
  using ParentComponent = ParentEntity<TagType>;

  if(childId == ECS::NULL_ENTITY)
  {
    return;
  }

  DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

  DCHECK(registry.has<ChildrenComponent>(childId));
  registry.remove<ChildrenComponent>(childId);

  DCHECK(registry.has<ParentComponent>(childId));
  registry.remove<ParentComponent>(childId);
}

} // namespace ECS
