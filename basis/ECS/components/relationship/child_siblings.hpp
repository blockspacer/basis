#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstdint>

namespace ECS {

// Represents element of hierarchy
//
// `ChildSiblings` component must be emplaced into child entity.
//
// Allows to represent hierarchies in ECS model
// without ruining the performance.
//
// A plus of this solution is that lists of children are
// implicitly defined in terms of components and you don not have
// to use an std::vector or similar for them.
// Therefore, you do not have dynamically allocated memory in your
// components in order to create a hierarchy.
//
/// \see https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
//
/// \note no guarantee that all the children are tightly
/// packed in memory, unless actions are taken in this regard
//
// `prev` and `next` are used to create
// an implicit double linked list of entities
/*
* Whenever you want to iterate all the children of an entity,
* this is the way to go:
*
* FirstChildInLinkedList* relationComp = registry.try_get<FirstChildInLinkedList>(parent_entity);
* ECS::Entity curr = relationComp.first;
* while(curr != ECS::NULL_ENTITY)
* {
*   // ...
*   ChildSiblings* currRelationComp = registry.try_get<ChildSiblings>(curr);
*   // Assume that all entities have the relationship component.
*   DCHECK(currRelationComp);
*   curr = currRelationComp->next;
* }
*/
//
// USAGE
//
// // Same entity may have multiple (different) lists of children entities like so:
// using ChildenNode = ChildSiblings<class NodeTag>;
// using ChildenWeaponGroup = ChildSiblings<class WeaponGroupTag>;
template <typename TagT>
struct ChildSiblings
{
  using TagType = TagT;

  // Previous sibling in the list of children for the parent.
  ECS::Entity prevId{ECS::NULL_ENTITY};

  // Next sibling in the list of children for the parent.
  ECS::Entity nextId{ECS::NULL_ENTITY};
};

} // namespace ECS
