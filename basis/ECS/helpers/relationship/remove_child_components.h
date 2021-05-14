#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>

#include <base/logging.h>

namespace ECS {

// remove all components associated with `child`
// (removes only components used to repesent child element in hierarchy)
template <
  typename TagType // unique type tag for all children
>
void removeChildComponents(
  ECS::Registry& registry
  , ECS::Entity childId)
{
  using ChildrenComponent = ChildSiblings<TagType>;
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
