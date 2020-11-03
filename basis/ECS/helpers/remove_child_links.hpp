#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

// Does not remove any components,
// only updates `prev` and `next` links in `ChildLinkedList` hierarchy
/// \note removes element only once (expects no duplicates)
/// \note does not remove `ChildLinkedList` component from child
/// \note does not remove `ParentEntity` component from child
/// \note does not remove child from parent components
/// (for example, does not update children count in parent)
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool removeChildLinks(
  ECS::Registry& registry
  , ECS::Entity childIdToRemove
  // starting element for search (search includes `listBeginId`)
  , ECS::Entity listBeginId = ECS::NULL_ENTITY
  // ending element for search (search includes `listEndId`)
  , ECS::Entity listEndId = ECS::NULL_ENTITY)
{
  using ChildrenComponent = ChildLinkedList<TagType>;

  if(childIdToRemove == ECS::NULL_ENTITY
     || listBeginId == ECS::NULL_ENTITY)
  {
    return false;
  }

  // starting element for search
  /// \note search includes `listBeginId`
  ECS::Entity curr = listBeginId;

  while(curr != ECS::NULL_ENTITY)
  {
    DCHECK_CHILD_ECS_ENTITY(curr, &registry, TagType);

    ChildrenComponent& currChildrenComp
      = registry.get<ChildrenComponent>(curr);

    ECS::Entity& currPrevId = currChildrenComp.prevId;

    ECS::Entity& currNextId = currChildrenComp.nextId;

    // found element to remove
    if(childIdToRemove == curr)
    {
      // prev. element must not point to removed element
      if(currPrevId != ECS::NULL_ENTITY)
      {
        DCHECK_ECS_ENTITY(currPrevId, &registry);

        DCHECK_CHILD_ECS_ENTITY(currPrevId, &registry, TagType);

        ChildrenComponent& prevChildrenComp
          = registry.get<ChildrenComponent>(currPrevId);

        prevChildrenComp.nextId = currNextId;
      }

      // next element must not point to removed element
      if(currNextId != ECS::NULL_ENTITY)
      {
        DCHECK_ECS_ENTITY(currNextId, &registry);

        DCHECK_CHILD_ECS_ENTITY(currNextId, &registry, TagType);

        ChildrenComponent& nextChildrenComp
          = registry.get<ChildrenComponent>(currNextId);

        nextChildrenComp.prevId = currPrevId;
      }

      return true;
    }

    curr = currChildrenComp.nextId;

    // ending element for search
    /// \note search includes `listEndId`
    if(curr == listEndId)
    {
      break;
    }
  }

  return false;
}

} // namespace ECS
