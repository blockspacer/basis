#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/helpers/relationship/is_child_at_top_level_of.h>
#include <basis/ECS/helpers/relationship/has_child_at_top_level.h>

#include <base/logging.h>

namespace ECS {

// Adds id of existing entity to linked list (at toplevel depth)
// as first element (modifies `FirstChildInLinkedList` component).
// Used to represent hierarchies in ECS model.
//
/// \note Assumes that `childId` does not have `ParentEntity`
/// with provided `TagType`
/// i.e. it can add only new child, not modify existing one
/// \note expects no child element duplication in linked list
/// \note Prefer to use it if order of children does not matter
/// (for performance reasons).
template <
  typename TagType // unique type tag for all children
>
void prependChildEntity(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childId)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ChildSiblings<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;
  using ParentComponent = ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY
     || childId == ECS::NULL_ENTITY)
  {
    return;
  }

  DCHECK(!isChildAtTopLevelOf<TagType>(REFERENCED(registry), parentId, childId));

  // sanity check
  DCHECK(parentId != childId);

  DCHECK_ECS_ENTITY(parentId, &registry);

  DCHECK_ECS_ENTITY(childId, &registry);

  FirstChildComponent* firstChild
    = registry.try_get<FirstChildComponent>(parentId);

  // mark child as part of linked list
  {
    CHECK(!registry.has<ChildrenComponent>(childId));
    registry.emplace<ChildrenComponent>(
      childId
      , ChildrenComponent{
          // `childId` will become first in list, so no `prev`
          ECS::NULL_ENTITY // prev
          // `childId` will become first in list,
          // so `next` must point to element
          // that was previous first in list (if list was not empty)
          , firstChild ? firstChild->firstId : ECS::NULL_ENTITY // next
        }
    );

    CHECK(!registry.has<ParentComponent>(childId));
    registry.emplace<ParentComponent>(
      childId
      , ParentComponent{parentId}
    );
  }

  if(firstChild)
  {
    DCHECK_ECS_ENTITY(firstChild->firstId, &registry);

    // increment size of linked list
    {
      DCHECK(registry.has<ChildrenSizeComponent>(parentId));
      ChildrenSizeComponent& childrenSize
        = registry.get<ChildrenSizeComponent>(parentId);
      /// \note runtime check (affects performance!)
      CHECK(childrenSize.size
        < std::numeric_limits<typename ChildrenSizeComponent::SizeType>::max())
        << "Unable to represent size of childrens in size_t";
      childrenSize.size++;
      // size can not be 0
      // because empty list do not have `ChildrenSizeComponent`
      DCHECK(childrenSize.size > 0);
    }

    // sanity check
    // first child must be marked child as part of linked list
    DCHECK(registry.has<ChildrenComponent>(firstChild->firstId));

    ChildrenComponent& children
      = registry.get<ChildrenComponent>(firstChild->firstId);

    // sanity check
    DCHECK(children.prevId == ECS::NULL_ENTITY);

    // change first element in list to `childId`
    children.prevId = childId;

    // change first element in list to `childId`
    firstChild->firstId = childId;
  } else {
    // set `childId` as first element in list
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    registry.emplace<FirstChildComponent>(
      parentId
      , FirstChildComponent{
          childId // firstId
        }
    );

    // set `childId` as first element in list
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));
    registry.emplace<ChildrenSizeComponent>(
      parentId
      , ChildrenSizeComponent{
          1 // only one element in list
        }
    );
  }

  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

  DCHECK(isChildAtTopLevelOf<TagType>(REFERENCED(registry), parentId, childId));

  DCHECK(hasChildAtTopLevel<TagType>(REFERENCED(registry), parentId, childId));

}

} // namespace ECS
