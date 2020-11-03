#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>

#include <base/logging.h>

namespace ECS {

template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
bool isChildEntity(
  ECS::Registry& registry
  , ECS::Entity entityId)
{
  using ChildrenComponent = ChildLinkedList<TagType>;
  using ParentComponent = ParentEntity<TagType>;

  return
    entityId != ECS::NULL_ENTITY
    && registry.has<ChildrenComponent>(entityId)
    && registry.has<ParentComponent>(entityId);
}

} // namespace ECS
