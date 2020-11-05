#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/helpers/has_parent_components.hpp>
#include <basis/ECS/helpers/has_child_components.hpp>
#include <basis/ECS/helpers/is_parent_of.hpp>

#include <base/logging.h>

namespace ECS {

// Unlike `hasChildInLinkedList` it will not iterate nodes in linked list,
// but check only `ParentComponent`.
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool isChildOf(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childId)
{
  return
    isParentOf<TagType>(REFERENCED(registry), parentId, childId);
}

} // namespace ECS
