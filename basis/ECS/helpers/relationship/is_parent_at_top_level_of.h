#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/helpers/relationship/has_parent_components.h>
#include <basis/ECS/helpers/relationship/has_child_components.h>
#include <basis/ECS/helpers/relationship/has_child_at_top_level.h>

#include <base/logging.h>
#include <base/macros.h>

#include <basic/macros.h>

namespace ECS {

/// \note does not iterate hierarchy recursively
/// i.e. does not iterate children of children of children...
//
// Unlike `hasChildAtTopLevel` it will not iterate nodes in linked list,
// but check only `ParentComponent`.
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool isParentAtTopLevelOf(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childId)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;
  using ChildrenComponent = ChildSiblings<TagType>;
  using ParentComponent = ParentEntity<TagType>;

  if(parentId == ECS::NULL_ENTITY
     || childId == ECS::NULL_ENTITY)
  {
    return false;
  }

  // check required components
  if(!hasParentComponents<TagType>(REFERENCED(registry), parentId))
  {
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    return false;
  }

  // check required components
  if(!hasChildComponents<TagType>(REFERENCED(registry), childId))
  {
    DCHECK(!registry.has<ChildrenComponent>(childId));
    DCHECK(!registry.has<ParentComponent>(childId));

    return false;
  }

  // check required components
  DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

  // check required components
  DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

  const bool isParentByComponent
    = registry.get<ParentComponent>(childId).parentId == parentId;

  DCHECK(isParentByComponent
    ? hasChildAtTopLevel<TagType>(REFERENCED(registry), parentId, childId)
    : !hasChildAtTopLevel<TagType>(REFERENCED(registry), parentId, childId));

  return isParentByComponent;
}

} // namespace ECS
