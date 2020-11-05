#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/helpers/has_parent_components.hpp>
#include <basis/ECS/helpers/has_child_components.hpp>
#include <basis/ECS/helpers/has_child_in_linked_list.hpp>
#include <base/logging.h>

namespace ECS {

// Unlike `hasChildInLinkedList` it will not iterate nodes in linked list,
// but check only `ParentComponent`.
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool isParentOf(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childId)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;
  using ChildrenComponent = ChildLinkedList<TagType>;
  using ParentComponent = ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY
     || childId == ECS::NULL_ENTITY)
  {
    return false;
  }

  // check required components
  if(!hasParentComponents<TagType>(REFERENCED(registry), parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    return false;
  }

  // check required components
  if(!hasChildComponents<TagType>(REFERENCED(registry), childId))
  {
    DCHECK(!registry.has<ChildrenComponent>(childId));
    DCHECK(!registry.has<ParentComponent>(childId));

    return false;
  }

  // check required components
  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  // check required components
  DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

  const bool isParentByComponent
    = registry.get<ParentComponent>(childId).parentId == parentId;

  DCHECK(isParentByComponent
    ? hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childId)
    : !hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childId));

  return isParentByComponent;
}

} // namespace ECS
