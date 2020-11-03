#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/helpers/is_parent_entity.hpp>

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
//
// USAGE
//
//  ECS::foreachChildEntity<TagType>(
//    REFERENCED(registry)
//    , parentEntityId
//    , base::BindRepeating(
//        [
//        ](
//          ECS::Entity parentEntityId
//          , ECS::Registry& registry
//          , ECS::Entity parentId
//          , ECS::Entity childId
//        ){
//          DCHECK(parentId != childId);
//
//          DCHECK_PARENT_ECS_ENTITY(parentId, &registry, TagType);
//        }
//        , parentEntityId
//      )
//    );
//  };
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
void foreachChildEntity(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ForeachChildEntityCb callback)
{
  using FirstChildComponent = ECS::FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ECS::ChildLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ECS::ChildLinkedListSize<TagType, size_t>;
  using ParentComponent = ECS::ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY)
  {
    return;
  }

  DCHECK_ECS_ENTITY(parentId, &registry);

  // sanity check
  DCHECK(callback);

  // check required components for parent that have any children
  if(!isParentEntity<TagType>(registry, parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    // no children i.e. nothing to do
    return;
  }

  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  DCHECK_CHILD_ECS_ENTITY(firstChild.firstId, &registry, TagType);

  ECS::Entity currChild = firstChild.firstId;

  // if removed element not first in list, then find it
  while(currChild != ECS::NULL_ENTITY)
  {
    DCHECK_ECS_ENTITY(currChild, &registry);

    // sanity check
    DCHECK(currChild != parentId);

    DCHECK_CHILD_ECS_ENTITY(currChild, &registry, TagType);

    ECS::Entity nextCurrChild = ECS::NULL_ENTITY;

    // cache some data because callback may free compoments
    {
      ChildrenComponent& currChildrenComp
        = registry.get<ChildrenComponent>(currChild);

      nextCurrChild = currChildrenComp.nextId;
    }

    /// \note callback may destroy `currChild` or any components
    callback.Run(REFERENCED(registry), parentId, currChild);

    // sanity check
    DCHECK((nextCurrChild != ECS::NULL_ENTITY)
      ? registry.valid(nextCurrChild)
      // no check for ECS::NULL_ENTITY
      : true);

    currChild = nextCurrChild;
  }
}

} // namespace ECS
