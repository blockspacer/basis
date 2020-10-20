#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstdint>

namespace ECS {

// Allows to represent hierarchies in ECS model
// without ruining the performance.
//
/// \see https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
//
// USAGE
//
// // Same entity may have multiple (different) lists of children entities like so:
// using ChildenSizeNode = ChildLinkedListSize<class NodeTag, size_t>;
// using ChildenSizeWeaponGroup = ChildLinkedListSize<class WeaponGroupTag, size_t>;
template <typename TagT, typename SizeT>
struct ChildLinkedListSize
{
  using SizeType = SizeT;
  using TagType = TagT;

  SizeType size{0};
};

} // namespace ECS
