#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

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
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;

  return
    entityId != ECS::NULL_ENTITY
    && registry.valid(entityId)
    && registry.has<FirstChildComponent>(entityId)
    && registry.has<ChildrenSizeComponent>(entityId);
}

} // namespace ECS
