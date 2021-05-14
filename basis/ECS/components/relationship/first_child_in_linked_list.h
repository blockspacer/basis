#pragma once

#include <basis/ECS/ecs.h>

#include <cstdint>

namespace ECS {

// Represents first element of hierarchy (at toplevel depth)
// (first element of linked list that stores children related to parent entity).
// `FirstChildInLinkedList` component must be emplaced into parent entity.
//
// Allows to represent hierarchies in ECS model
// without ruining the performance.
/// \see https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
//
// USAGE
//
// // Same entity may be child of multiple (different) entities like so:
// using FirstNode = FirstChildInLinkedList<class NodeTag>;
// using FirstWeaponGroup = FirstChildInLinkedList<class WeaponGroupTag>;
template <typename TagT>
CREATE_ECS_COMPONENT(FirstChildInLinkedList)
{
  using TagType = TagT;

  // Entity identifier of the first child, if any.
  ECS::Entity firstId{ECS::NULL_ENTITY};
};

} // namespace ECS

ECS_DECLARE_METATYPE_TEMPLATE_1ARG(FirstChildInLinkedList);
