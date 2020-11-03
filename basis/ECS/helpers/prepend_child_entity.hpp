#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

// Adds id of existing entity to linked list
// as first element (NOT as last element as you may want!).
// Used to represent hierarchies in ECS model.
//
/// \note Order of children may be corrupted, use with caution (!!!).
/// Prefer to use it if order of children does not matter
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
  using ChildrenComponent = ChildLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;
  using ParentComponent = ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY
     || childId == ECS::NULL_ENTITY)
  {
    return;
  }

  DVLOG(99)
    << " prepended child entity "
    << childId
    << " to parent entity "
    << parentId;

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
}

} // namespace ECS
