#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/relationship/child_siblings.hpp>
#include <basis/ECS/components/relationship/first_child_in_linked_list.hpp>
#include <basis/ECS/components/relationship/parent_entity.hpp>
#include <basis/ECS/components/relationship/top_level_children_count.hpp>
#include <basis/ECS/helpers/relationship/has_parent_components.hpp>

#include <base/logging.h>

namespace ECS {

/// \note does not iterate hierarchy recursively
/// i.e. does not iterate children of children of children...
//
// Unlike `isParentAtTopLevelOf` or `isChildAtTopLevelOf` it will iterate all
// nodes in linked list (only at top level) until `childIdToFind` found,
// even if `ParentComponent` alrady points to `parentId`.
/// \note prefer `isParentAtTopLevelOf` or `isChildAtTopLevelOf` because of performance reasons
/// (you can check if element is part of hierarchy without iteration).
/// \note returns `false` if child not found
/// \note expects no child element duplication in linked list
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool hasChildAtTopLevel(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childIdToFind)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ChildSiblings<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;
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
    DCHECK_EQ(registry.get<ParentComponent>(childIdToFind).parentId, parentId);
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
      DCHECK_EQ(registry.get<ParentComponent>(childIdToFind).parentId, parentId);
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
