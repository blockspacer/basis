#pragma once

#include <entt/entt.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/group.hpp>

#include <cstddef>
#include <cstdint>
#include <sstream>

// using a hashed string under VS could generate a warning.
// it's expected and harmless. However, it can be annoying.
// To suppress it and if you don't want to suppress all the other warnings as well,
// here is a workaround in the form of a macro:
// USAGE:
//   constexpr auto identifier = HS("my/resource/identifier");
#if defined(_MSC_VER)
#define HASHED_STR(str) \
  __pragma(warning(suppress:4307)) entt::hashed_string{str}
#else // defined(_MSC_VER)
#define HASHED_STR(str) \
  entt::hashed_string{str}
#endif // defined(_MSC_VER)

namespace ECS {

// Custom entity identifiers are definitely a good idea in two cases at least:
// 1. If std::uint32_t isn't large enough as an underlying type.
// 2. If you want to avoid conflicts when using multiple registries.
// The use of this macro is highly recommended,
// so as not to run into problems if the requirements
// for the identifiers should change in the future.
ENTT_OPAQUE_TYPE(EntityId, std::uint32_t);

// Registry stores entities and arranges pools of components
using Registry = entt::basic_registry<ECS::EntityId>;

// Underlying entity identifier
using Entity = ECS::EntityId;

static const Entity NULL_ENTITY = entt::null;

// allow to print entity identifier
inline std::ostream& operator<<(
  std::ostream& stream, Entity entity_id)
{
  return stream
    // |to_integral| defined by |ENTT_OPAQUE_TYPE|
    << to_integral(entity_id);
}

#define ECS_LABEL(__name__) using __name__ = entt::tag<#__name__ ## _hs>

#define CREATE_ECS_TAG(TAG_NAME) \
  ECS_LABEL(TAG_NAME);

} // namespace ECS
