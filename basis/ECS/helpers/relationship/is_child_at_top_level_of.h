﻿#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/helpers/relationship/has_parent_components.h>
#include <basis/ECS/helpers/relationship/has_child_components.h>
#include <basis/ECS/helpers/relationship/is_parent_at_top_level_of.h>

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
bool isChildAtTopLevelOf(
  ECS::Registry& registry
  , ECS::Entity parentId
  , ECS::Entity childId)
{
  return
    isParentAtTopLevelOf<TagType>(REFERENCED(registry), parentId, childId);
}

} // namespace ECS
