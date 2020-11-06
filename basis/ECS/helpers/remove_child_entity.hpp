#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/helpers/has_parent_components.hpp>
#include <basis/ECS/helpers/has_child_components.hpp>
#include <basis/ECS/helpers/remove_child_components.hpp>
#include <basis/ECS/helpers/remove_child_links.hpp>
#include <basis/ECS/helpers/remove_parent_components.hpp>
#include <basis/ECS/helpers/has_child_in_linked_list.hpp>
#include <basis/ECS/helpers/is_child_of.hpp>

#include <base/logging.h>

namespace ECS {

// Removes id of existing entity from linked list
// and modifies components in parent and child entity.
//
// Returns `false` if entity can not be removed.
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

  if(childIdToRemove == ECS::NULL_ENTITY
     || parentId == ECS::NULL_ENTITY)
  {
    return false;
  }

  DCHECK_ECS_ENTITY(childIdToRemove, &registry);

  DCHECK_ECS_ENTITY(parentId, &registry);

  DCHECK(parentId != childIdToRemove);

  // check required components
  if(!hasParentComponents<TagType>(REFERENCED(registry), parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    return false;
  }

  // check required components
  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  FirstChildComponent& firstChild
    = registry.get<FirstChildComponent>(parentId);

  // sanity check
  DCHECK_CHILD_ENTITY_COMPONENTS(firstChild.firstId, &registry, TagType);

  // check required components
  if(!hasChildComponents<TagType>(REFERENCED(registry), childIdToRemove))
  {
    DCHECK(!registry.has<ChildrenComponent>(childIdToRemove));
    DCHECK(!registry.has<ParentComponent>(childIdToRemove));

    DCHECK_NE(childIdToRemove, firstChild.firstId);

    DCHECK(!hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childIdToRemove));

    return false;
  }

  // sanity check
  DCHECK_CHILD_ENTITY_COMPONENTS(childIdToRemove, &registry, TagType);

  ChildrenComponent& childrenCompToRemove
    = registry.get<ChildrenComponent>(childIdToRemove);

  if(!isChildOf<TagType>(REFERENCED(registry), parentId, childIdToRemove))
  {
    DCHECK(!hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childIdToRemove));

    DCHECK_NE(childIdToRemove, firstChild.firstId);

    // `childIdToRemove` not found i.e. nothing to do
    return false;
  }

  /// \note change `firstChild` 
  /// before modifications in `ChildLinkedList` hierarchy
  if(childIdToRemove == firstChild.firstId)
  {
    DCHECK(hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childIdToRemove));

    // mark as first element in list
    firstChild.firstId = childrenCompToRemove.nextId;

    // no more children related to `parentId`
    if(childrenCompToRemove.nextId == ECS::NULL_ENTITY)
    {
      DCHECK(registry.has<ChildrenSizeComponent>(parentId));
      ChildrenSizeComponent& childrenSize
        = registry.get<ChildrenSizeComponent>(parentId);
      DCHECK_EQ(childrenSize.size, 1UL);
    }
  }

  // update `prev` and `next` links in `ChildLinkedList` hierarchy
  bool isRemovedFromListLinks
    = removeChildLinks<TagType>(
        REFERENCED(registry)
        , childIdToRemove // childIdToRemove
        // because `childIdToRemove` same as `listBeginId`
        // it will take 1 iteraton to perform `removeChildLinks`
        , childIdToRemove // listBeginId
        , ECS::NULL_ENTITY // listEndId
      );

  if(childIdToRemove == firstChild.firstId)
  {
    DCHECK(isRemovedFromListLinks); // child entity found in list
  }

  if(!isRemovedFromListLinks)
  {
    // `childIdToRemove` not found i.e. nothing to do
    return false;
  }

  // if `childIdToRemove` found, then it must have child components
  DCHECK_CHILD_ENTITY_COMPONENTS(childIdToRemove, &registry, TagType);

  // decrement size of linked list,
  // but only if child entity found
  {
    DCHECK(isRemovedFromListLinks); // child entity found in list
    DCHECK(registry.has<ChildrenSizeComponent>(parentId));
    ChildrenSizeComponent& childrenSize
      = registry.get<ChildrenSizeComponent>(parentId);
    childrenSize.size--;
    // size can not be 0
    // because empty list do not have `ChildrenSizeComponent`
    if(childrenSize.size <= 0)
    {
      // remove all components associated with `parent`
      removeParentComponents<TagType>(
        REFERENCED(registry)
        , parentId
      );
    }
  }

  // child no more in list,
  // remove all components associated with `child`
  removeChildComponents<TagType>(
    REFERENCED(registry)
    , childIdToRemove
  );

  // child was removed from list
  DCHECK(!hasChildInLinkedList<TagType>(REFERENCED(registry), parentId, childIdToRemove));

  // child was removed from parent components
  DCHECK(!isChildOf<TagType>(REFERENCED(registry), parentId, childIdToRemove));

  return true;
}

} // namespace ECS
