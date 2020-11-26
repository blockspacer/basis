#pragma once

#include <entt/entt.hpp> // IWYU pragma: keep
#include <entt/entity/registry.hpp> // IWYU pragma: keep
#include <entt/entity/helper.hpp> // IWYU pragma: keep
#include <entt/entity/entity.hpp> // IWYU pragma: keep
#include <entt/core/type_info.hpp> // IWYU pragma: keep
#include <entt/core/type_traits.hpp> // IWYU pragma: keep
#include <entt/entity/group.hpp> // IWYU pragma: keep

#include <base/logging.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <type_traits>

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

/// \note `struct EntityId` default initialized with `entt::null`
/// to avoid bugs similar to github.com/skypjack/entt/issues/561
// Custom entity identifiers are definitely a good idea in two cases at least:
// 1. If std::uint32_t isn't large enough as an underlying type.
// 2. If you want to avoid conflicts when using multiple registries.
// The use of this macro is highly recommended,
// so as not to run into problems if the requirements
// for the identifiers should change in the future.
struct EntityId
{
  using traits_type = entt::entt_traits<entt::entity>;

  using entity_type
    = typename traits_type::entity_type;

  EntityId(entity_type value = entt::null)
    : entt{value}
  {}

  EntityId(const EntityId& other)
    : entt{other.entt}
  {}

  operator entity_type() const {
    return entt;
  }

private:
  entity_type entt;
};

} // namespace ECS

namespace entt {

template<>
struct entt_traits<ECS::EntityId>: entt_traits<entt::entity> {};

} // namespace entt

namespace ECS {

// Registry stores entities and arranges pools of components
using Registry
  = entt::basic_registry<ECS::EntityId>;

// Alias for
// template<typename Entity, typename... Exclude, typename... Component>
// class basic_view<Entity, exclude_t<Exclude...>, Component...>
template<typename... Args>
using View
  = entt::basic_view<
      ECS::EntityId
      , Args...
    >;

// Underlying entity identifier
using Entity
  = ECS::EntityId;

static const Entity NULL_ENTITY
  = entt::null;

template<typename... Type>
struct exclude_t: entt::type_list<Type...> {};

template<typename... Type>
inline constexpr exclude_t<Type...> exclude{};

template<typename... Type>
struct include_t: entt::type_list<Type...> {};

template<typename... Type>
inline constexpr include_t<Type...> include{};

template<typename... Type>
struct get_t: entt::type_list<Type...>{};

template<typename... Type>
inline constexpr get_t<Type...> get{};

// allow to print entity identifier
inline std::ostream& operator<<(
  std::ostream& stream, Entity entity_id)
{
  return stream
    // |to_integral| defined by |ENTT_OPAQUE_TYPE|
    << entt::to_integral(entity_id);
}

#define ECS_LABEL(__name__) \
  using __name__ = entt::tag<#__name__ ## _hs>

#define CREATE_ECS_TAG(TAG_NAME) \
  ECS_LABEL(TAG_NAME);

#define LOCATION_ECS_TAG_NAME1(x) \
  #x

#define LOCATION_ECS_TAG_NAME2(x) \
  LOCATION_ECS_TAG_NAME1(x)

#define LOCATION_ECS_TAG_NAME(TAG_NAME) \
  LOCATION_ECS_TAG_NAME2(TAG_NAME) " : " \
  __FILE__ " : " \
  LOCATION_ECS_TAG_NAME2(__LINE__)

#define CREATE_UNIQUE_ECS_TAG(TAG_NAME) \
  ECS_LABEL(LOCATION_ECS_TAG_NAME(TAG_NAME));

// USAGE
//
// DCHECK_ECS_ENTITY(entityId, &registry);
// DCHECK_ECS_ENTITY(entityId, registryStrongAlias, TagType);
#define DCHECK_ECS_ENTITY(__name__, __registry_ptr__) \
  { \
    DCHECK(__name__ != ECS::NULL_ENTITY); \
    DCHECK((__registry_ptr__)->valid(__name__)); \
  }

// check that child entity has all required components
//
// USAGE
//
// DCHECK_CHILD_ENTITY_COMPONENTS(entityId, &registry, TagType);
// DCHECK_CHILD_ENTITY_COMPONENTS(entityId, registryStrongAlias, TagType);
#define DCHECK_CHILD_ENTITY_COMPONENTS(__name__, __registry_ptr__, __tag_type__) \
  { \
    DCHECK_ECS_ENTITY(__name__, __registry_ptr__); \
    DCHECK((__registry_ptr__)->has<ParentEntity<__tag_type__>>(__name__)); \
    DCHECK((__registry_ptr__)->has<ChildSiblings<__tag_type__>>(__name__)); \
  }

// check that parent entity has all required components
//
// USAGE
//
// DCHECK_PARENT_ENTITY_COMPONENTS(entityId, &registry, TagType);
// DCHECK_PARENT_ENTITY_COMPONENTS(entityId, registryStrongAlias, TagType);
#define DCHECK_PARENT_ENTITY_COMPONENTS(__name__, __registry_ptr__, __tag_type__) \
  { \
    DCHECK_ECS_ENTITY(__name__, __registry_ptr__); \
    bool hasFirstChild \
      = (__registry_ptr__)->has<FirstChildInLinkedList<__tag_type__>>(__name__); \
    DCHECK(hasFirstChild); \
    bool hasTopLevelChildrenCount \
      = (__registry_ptr__)->has<TopLevelChildrenCount<__tag_type__, size_t>>(__name__); \
    DCHECK(hasTopLevelChildrenCount); \
  }
} // namespace ECS
