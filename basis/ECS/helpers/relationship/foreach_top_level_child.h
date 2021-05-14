#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/helpers/relationship/has_parent_components.h>

#include <base/logging.h>
#include <base/callback.h>
#include <base/macros.h>

#include <basic/macros.h>

namespace ECS {

using foreachTopLevelChildCb
  = ::base::RepeatingCallback<
      void(ECS::Registry&, ECS::Entity parentId, ECS::Entity childId)
    >;

/// \note does not iterate hierarchy recursively
/// i.e. does not iterate children of children of children...
//
// Iterates each entity in linked list
// and calls callback on each iterated entity.
//
// USAGE
//
//  ECS::foreachTopLevelChild<TagType>(
//    REFERENCED(registry)
//    , parentEntityId
//    , ::base::BindRepeating(
//        [
//        ](
//          ECS::Entity parentEntityId
//          , ECS::Registry& registry
//          , ECS::Entity parentId
//          , ECS::Entity childId
//        ){
//          DCHECK(parentId != childId);
//
//          DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);
//        }
//        , parentEntityId
//      )
//    );
//  };
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
void foreachTopLevelChild(
  ECS::Registry& registry
  , ECS::Entity parentId
  , foreachTopLevelChildCb callback)
{
  using FirstChildComponent = ECS::FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ECS::ChildSiblings<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ECS::TopLevelChildrenCount<TagType, size_t>;

  if(parentId == ECS::NULL_ENTITY)
  {
    return;
  }

  DCHECK_ECS_ENTITY(parentId, &registry);

  // sanity check
  DCHECK(callback);

  // check required components
  if(!hasParentComponents<TagType>(REFERENCED(registry), parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    return;
  }

  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  DCHECK_CHILD_ENTITY_COMPONENTS(firstChild.firstId, &registry, TagType);

  ECS::Entity currChild = firstChild.firstId;

  // if removed element not first in list, then find it
  while(currChild != ECS::NULL_ENTITY)
  {
    DCHECK_ECS_ENTITY(currChild, &registry);

    // sanity check
    DCHECK(currChild != parentId);

    DCHECK_CHILD_ENTITY_COMPONENTS(currChild, &registry, TagType);

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
