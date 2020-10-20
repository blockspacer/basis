#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstdint>

namespace ECS {

// Allows to represent hierarchies in ECS model
// without ruining the performance.
/// \see https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
/*
 * Whenever you want to go back to the parent from a given entity,
 * this is the way to go:
 *
 * ParentEntity* relationComp = registry.try_get<ParentEntity>(entity);
 * if(relationComp && relationComp->parent != ECS::NULL_ENTITY)
 * {
 *   // ...
 * }
*/
//
// USAGE
//
// // Same entity may multiple (different) parent entities like so:
// using ParentNode = ParentEntity<class NodeTag>;
// using ParentWeaponGroup = ParentEntity<class WeaponGroupTag>;
template <typename TagT>
struct ParentEntity
{
  using TagType = TagT;

  // Entity identifier of the parent, if any.
  ECS::Entity parentId{ECS::NULL_ENTITY};
};

} // namespace ECS
