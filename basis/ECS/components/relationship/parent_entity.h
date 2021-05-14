#pragma once

#include <basis/ECS/ecs.h>

#include <cstdint>

namespace ECS {

// Stores id of parent element in hierarchy
// (parent element must be associated with linked list of children).
// `ParentEntity` component must be emplaced into child entity.
//
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
CREATE_ECS_COMPONENT(ParentEntity)
{
  using TagType = TagT;

  // Entity identifier of the parent, if any.
  ECS::Entity parentId{ECS::NULL_ENTITY};
};

} // namespace ECS

ECS_DECLARE_METATYPE_TEMPLATE_1ARG(ParentEntity);
