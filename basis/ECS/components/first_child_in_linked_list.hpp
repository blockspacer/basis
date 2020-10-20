#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstdint>

namespace ECS {

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
struct FirstChildInLinkedList
{
  using TagType = TagT;

  // Entity identifier of the first child, if any.
  ECS::Entity firstId{ECS::NULL_ENTITY};
};

} // namespace ECS
