#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

// remove all components associated with `parent`
template <
  typename TagType // unique type tag for all children
>
void removeParentComponents(
  ECS::Registry& registry
  , ECS::Entity parentId)
{
  using FirstChildComponent = ECS::FirstChildInLinkedList<TagType>;
  using ChildrenSizeComponent = ECS::ChildLinkedListSize<TagType, size_t>;

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
