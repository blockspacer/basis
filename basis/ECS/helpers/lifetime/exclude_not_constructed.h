#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/tags.h>

#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/utility.hpp>

namespace ECS {

// Adds to `entt::exclude` tags that mark entity as invalid
// (for example, entity is invalid if it is scheduled for destruction).
//
// USAGE
//
//  auto registry_group
//    = registry->view<view_component>(
//        ECS::exclude_not_constructed<
//          ECS::MyExtraTag
//          , ECS::MyOtherTag
//        >
//      );
//
template<typename... Type>
inline constexpr
  entt::exclude_t<
    // entity in destruction
    ECS::NeedToDestroyTag
    // entity not fully created
    , ECS::DelayedConstruction
    // entity is unused i.e. may be invalid (used by memory pool i.e. cache)
    , ECS::UnusedTag
    , Type...
  > exclude_not_constructed{};

} // namespace ECS
