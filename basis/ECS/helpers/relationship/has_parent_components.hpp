#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/relationship/child_siblings.hpp>
#include <basis/ECS/components/relationship/first_child_in_linked_list.hpp>
#include <basis/ECS/components/relationship/parent_entity.hpp>
#include <basis/ECS/components/relationship/top_level_children_count.hpp>

#include <base/logging.h>

namespace ECS {

// checks components required by any `Parent`
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool hasParentComponents(
  ECS::Registry& registry
  , ECS::Entity entityId)
{
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;

  return
    entityId != ECS::NULL_ENTITY
    && registry.valid(entityId)
    && registry.has<FirstChildComponent>(entityId)
    && registry.has<ChildrenSizeComponent>(entityId);
}

} // namespace ECS
