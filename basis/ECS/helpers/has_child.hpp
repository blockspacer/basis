#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/helpers/is_parent_entity.hpp>

#include <base/logging.h>

namespace ECS {

/// \note returns `ECS::NULL_ENTITY` if child not found
/// \note returns first found element (expects no duplicates)
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool hasChild(
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

  // check required components for parent that have any children
  if(!isParentEntity<TagType>(registry, parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    // no children i.e. nothing to do
    return false;
  }

  DCHECK(registry.has<FirstChildComponent>(parentId));
  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  DCHECK_ECS_ENTITY(firstChild.firstId, &registry);

  if(childIdToFind == firstChild.firstId)
  {
    DCHECK_CHILD_ECS_ENTITY(firstChild.firstId, &registry, TagType);
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

    DCHECK_CHILD_ECS_ENTITY(curr, &registry, TagType);

    // found element
    if(childIdToFind == curr)
    {
      DCHECK_CHILD_ECS_ENTITY(curr, &registry, TagType);
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
