#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstdint>

namespace ECS {

// Represents size of children (at toplevel depth) related to parent entity.
// (size of linked list that stores toplevel children related to parent entity).
// `TopLevelChildrenCount` component must be emplaced into parent entity.
//
/// \see https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
//
// USAGE
//
// // Same entity may have multiple (different) lists of children entities like so:
// using ChildenSizeNode = TopLevelChildrenCount<class NodeTag, size_t>;
// using ChildenSizeWeaponGroup = TopLevelChildrenCount<class WeaponGroupTag, size_t>;
template <typename TagT, typename SizeT>
CREATE_ECS_COMPONENT(TopLevelChildrenCount)
{
  using SizeType = SizeT;
  using TagType = TagT;

  SizeType size{0};
};

} // namespace ECS

ECS_DECLARE_METATYPE_TEMPLATE_2ARG(TopLevelChildrenCount);
