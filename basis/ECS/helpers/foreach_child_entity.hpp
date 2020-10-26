#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>

#include <base/logging.h>
#include <base/callback.h>

namespace ECS {

using ForeachChildEntityCb
  = base::RepeatingCallback<
      void(ECS::Registry&, ECS::Entity parentId, ECS::Entity childId)
    >;

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
void foreachChildEntity(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ForeachChildEntityCb callback)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ChildLinkedList<TagType>;

  // sanity check
  DCHECK(parentId != ECS::NULL_ENTITY);

  // sanity check
  DCHECK(registry.valid(parentId));

  // sanity check
  DCHECK(callback);

  if(!registry.has<FirstChildComponent>(parentId))
  {
    // no children i.e. nothing to do
    return;
  }

  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  // sanity check
  DCHECK(firstChild.firstId != ECS::NULL_ENTITY);

  ECS::Entity curr = firstChild.firstId;

  // if removed element not first in list, then find it
  while(curr != ECS::NULL_ENTITY)
  {
    // sanity check
    DCHECK(registry.valid(curr));

    ECS::Entity nextCurr = ECS::NULL_ENTITY;

    // cache some data because callback may free compoments
    {
      // Assume that all entities have the relationship component.
      DCHECK(registry.has<ChildrenComponent>(curr));

      ChildrenComponent& currChildrenComp
        = registry.get<ChildrenComponent>(curr);

      nextCurr = currChildrenComp.nextId;
    }

    /// \note callback may destroy `curr` or any components
    callback.Run(REFERENCED(registry), parentId, curr);

    // sanity check
    DCHECK((nextCurr != ECS::NULL_ENTITY)
      ? registry.valid(nextCurr)
      : true);

    curr = nextCurr;
  }
}

} // namespace ECS
