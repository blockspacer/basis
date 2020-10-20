#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

// Removes id of existing entity from linked list.
//
// Returns `false` if entity not found linked list
// i.e. can not be removed.
//
// Used to represent hierarchies in ECS model.
template <
  typename TagType  // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool removeChildEntity(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childIdToRemove)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ChildLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;
  using ParentComponent = ParentEntity<TagType>;

  // sanity check
  DCHECK(registry.valid(parentId));

  // sanity check
  DCHECK(registry.valid(childIdToRemove));

  if(!registry.has<FirstChildComponent>(parentId))
  {
    // no children i.e. nothing to do
    return false;
  }

  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  // sanity check
  DCHECK(firstChild.firstId != ECS::NULL_ENTITY);

  if(childIdToRemove == firstChild.firstId)
  {
    // Assume that all entities have the relationship component.
    DCHECK(registry.has<ChildrenComponent>(childIdToRemove));

    ChildrenComponent& childrenCompToRemove
      = registry.get<ChildrenComponent>(childIdToRemove);

    if(childrenCompToRemove.nextId == ECS::NULL_ENTITY)
    {
      // no more children related to `parentId`
      registry.remove<FirstChildComponent>(parentId);
      registry.remove<ChildrenSizeComponent>(parentId);
    } else {
      // mark as first element in list
      firstChild.firstId = childrenCompToRemove.nextId;

      ChildrenComponent& nextChildrenComp
        = registry.get<ChildrenComponent>(childrenCompToRemove.nextId);

      // sanity check
      DCHECK(nextChildrenComp.prevId == childIdToRemove);

      // mark as first element in list
      nextChildrenComp.prevId = ECS::NULL_ENTITY;

      // decrement size of linked list
      {
        DCHECK(registry.has<ChildrenSizeComponent>(parentId));
        ChildrenSizeComponent& childrenSize
          = registry.get<ChildrenSizeComponent>(parentId);
        childrenSize.size--;
        // size can not be 0
        // because empty list do not have `ChildrenSizeComponent`
        DCHECK(childrenSize.size > 0);
      }
    }

    // child no more in list
    {
      registry.remove<ChildrenComponent>(childIdToRemove);
      registry.remove<ParentComponent>(childIdToRemove);
    }

    // `childIdToRemove` removed without errors
    return true;
  }

  ChildrenComponent& childrenFirstChild
    = registry.get<ChildrenComponent>(firstChild.firstId);

  // we handle first element separately, so skip `firstChild.firstId`
  ECS::Entity curr = childrenFirstChild.nextId;

  // if removed element not first in list, then find it
  while(curr != ECS::NULL_ENTITY)
  {
    // sanity check
    DCHECK(registry.valid(curr));

    // Assume that all entities have the relationship component.
    DCHECK(registry.has<ChildrenComponent>(curr));

    ChildrenComponent& currChildrenComp
      = registry.get<ChildrenComponent>(curr);

    ECS::Entity& currPrevId = currChildrenComp.prevId;

    ECS::Entity& currNextId = currChildrenComp.nextId;

    if(childIdToRemove == curr)
    {
      if(currPrevId != ECS::NULL_ENTITY)
      {
        // sanity check
        DCHECK(registry.valid(currPrevId));

        ChildrenComponent& prevChildrenComp
          = registry.get<ChildrenComponent>(currPrevId);

        prevChildrenComp.nextId = currNextId;
      }

      if(currNextId != ECS::NULL_ENTITY)
      {
        // sanity check
        DCHECK(registry.valid(currNextId));

        ChildrenComponent& nextChildrenComp
          = registry.get<ChildrenComponent>(currNextId);

        nextChildrenComp.prevId = currPrevId;
      }

      // child no more in list
      {
        registry.remove<ChildrenComponent>(childIdToRemove);
        registry.remove<ParentComponent>(childIdToRemove);

        // decrement size of linked list
        {
          DCHECK(registry.has<ChildrenSizeComponent>(parentId));
          ChildrenSizeComponent& childrenSize
            = registry.get<ChildrenSizeComponent>(parentId);
          childrenSize.size--;
          // size can not be 0
          // because empty list do not have `ChildrenSizeComponent`
          DCHECK(childrenSize.size > 0);
        }
      }

      // `childIdToRemove` removed without errors
      return true;
    }

    curr = currChildrenComp.nextId;
  }

  // `childIdToRemove` not found i.e. nothing to do
  return false;
}

} // namespace ECS
