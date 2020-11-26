#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/relationship/child_siblings.hpp>
#include <basis/ECS/components/relationship/first_child_in_linked_list.hpp>
#include <basis/ECS/components/relationship/parent_entity.hpp>
#include <basis/ECS/components/relationship/top_level_children_count.hpp>

#include <base/logging.h>

namespace ECS {

// remove all components associated with `parent`
// (removes only components used to repesent parent element in hierarchy)
template <
  typename TagType // unique type tag for all children
>
void removeParentComponents(
  ECS::Registry& registry
  , ECS::Entity parentId)
{
  using FirstChildComponent = ECS::FirstChildInLinkedList<TagType>;
  using ChildrenSizeComponent = ECS::TopLevelChildrenCount<TagType, size_t>;

  if(parentId == ECS::NULL_ENTITY)
  {
    return;
  }

  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  DCHECK(registry.has<FirstChildComponent>(parentId));
  registry.remove<FirstChildComponent>(parentId);

  DCHECK(registry.has<ChildrenSizeComponent>(parentId));
  registry.remove<ChildrenSizeComponent>(parentId);
}

} // namespace ECS
