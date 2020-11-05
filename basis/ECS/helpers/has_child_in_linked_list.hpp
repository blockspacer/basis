#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/helpers/has_parent_components.hpp>

#include <base/logging.h>

namespace ECS {

// Unlike `isParentOf` or `isChildOf` it will iterate all
// nodes in linked list until `childIdToFind` found,
// even if `ParentComponent` alrady points to `parentId`.
/// \note prefer `isParentOf` or `isChildOf` because of
/// performance reasons.
/// \note returns `false` if child not found
/// \note expects no child element duplication in linked list
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool hasChildInLinkedList(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childIdToFind)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ChildLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;
  using ParentComponent = ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY
     || childIdToFind == ECS::NULL_ENTITY)
  {
    return false;
  }

  DCHECK_ECS_ENTITY(parentId, &registry);

  DCHECK_ECS_ENTITY(childIdToFind, &registry);

  // check required components
  if(!hasParentComponents<TagType>(REFERENCED(registry), parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    return false;
  }

  DCHECK(registry.has<FirstChildComponent>(parentId));
  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  DCHECK_ECS_ENTITY(firstChild.firstId, &registry);

  if(childIdToFind == firstChild.firstId)
  {
    DCHECK_CHILD_ENTITY_COMPONENTS(firstChild.firstId, &registry, TagType);
    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    return true;
  }

  ChildrenComponent& childrenFirstChild
    = registry.get<ChildrenComponent>(firstChild.firstId);

  // we handle first element separately, so skip `firstChild.firstId`
  ECS::Entity curr = childrenFirstChild.nextId;

  // if element not first in list, then find it
  while(curr != ECS::NULL_ENTITY)
  {
    DCHECK_ECS_ENTITY(curr, &registry);

    DCHECK_CHILD_ENTITY_COMPONENTS(curr, &registry, TagType);

    // found element
    if(childIdToFind == curr)
    {
      DCHECK_CHILD_ENTITY_COMPONENTS(curr, &registry, TagType);
      DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
      return true;
    }

    ChildrenComponent& currChildrenComp
      = registry.get<ChildrenComponent>(curr);

    curr = currChildrenComp.nextId;
  }

  // not found i.e. nothing to do
  return false;
}

} // namespace ECS
