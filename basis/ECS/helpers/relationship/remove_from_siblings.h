#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/helpers/relationship/is_child_at_top_level_of.h>

#include <base/logging.h>
#include <base/macros.h>

#include <basic/macros.h>

namespace ECS {

// Does not remove any components,
// only updates `prev` and `next` links in `ChildSiblings` hierarchy
// i.e. you must remove some components from parent and child entity manually
/// \note expects no duplicates
/// \note does not remove `ChildSiblings` component from child
/// \note does not remove `ParentEntity` component from child
/// \note does not remove child from parent components
/// (for example, does not update children count in parent)
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool removeFromSiblings(
  ECS::Registry& registry
  , ECS::Entity childIdToRemove
  // starting element for search (search includes `listBeginId`)
  , ECS::Entity listBeginId = ECS::NULL_ENTITY
  // ending element for search (search includes `listEndId`)
  , ECS::Entity listEndId = ECS::NULL_ENTITY)
{
  using ChildrenComponent = ChildSiblings<TagType>;
  using ParentComponent = ParentEntity<TagType>;

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
    DCHECK_CHILD_ENTITY_COMPONENTS(curr, &registry, TagType);

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

        DCHECK_CHILD_ENTITY_COMPONENTS(currPrevId, &registry, TagType);

        // sanity check: siblings must have same parent
        DCHECK_EQ(registry.get<ParentComponent>(currPrevId).parentId
          , registry.get<ParentComponent>(curr).parentId);

        ChildrenComponent& prevChildrenComp
          = registry.get<ChildrenComponent>(currPrevId);

        prevChildrenComp.nextId = currNextId;
      }

      // next element must not point to removed element
      if(currNextId != ECS::NULL_ENTITY)
      {
        DCHECK_ECS_ENTITY(currNextId, &registry);

        DCHECK_CHILD_ENTITY_COMPONENTS(currNextId, &registry, TagType);

        // sanity check: siblings must have same parent
        DCHECK_EQ(registry.get<ParentComponent>(currNextId).parentId
          , registry.get<ParentComponent>(curr).parentId);

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
