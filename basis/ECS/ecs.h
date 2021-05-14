#pragma once

#include <entt/entt.hpp> // IWYU pragma: keep
#include <entt/entity/registry.hpp> // IWYU pragma: keep
#include <entt/entity/helper.hpp> // IWYU pragma: keep
#include <entt/entity/entity.hpp> // IWYU pragma: keep
#include <entt/core/type_info.hpp> // IWYU pragma: keep
#include <entt/core/type_traits.hpp> // IWYU pragma: keep
#include <entt/entity/group.hpp> // IWYU pragma: keep
#include <entt/core/hashed_string.hpp> // IWYU pragma: keep

#include <base/macros.h>
#include <base/logging.h>

#include <basic/macros.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <type_traits>
#include <string>

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

template<typename... Type>
struct remove_t: entt::type_list<Type...> {};

template<typename... Type>
inline constexpr remove_t<Type...> remove{};

template<typename... Type>
struct emplace_t: entt::type_list<Type...> {};

template<typename... Type>
inline constexpr emplace_t<Type...> emplace{};

// allow to print entity identifier
inline std::ostream& operator<<(
  std::ostream& stream, Entity entity_id)
{
  return stream
    // |to_integral| defined by |ENTT_OPAQUE_TYPE|
    << entt::to_integral(entity_id);
}

// Meta information about ECS component or ECS tag that can be
// accessed at run time.
// Used by `ECS_DECLARE_METATYPE`
struct TypeMeta {
  std::string name{};
};

// Used by `ECS_DECLARE_METATYPE`
//
// USAGE
//
// DCHECK(!ECS::setOrFindTypeMeta(entt::type_info<ECS::UnusedTag>::id(), ...).name.empty());
//
TypeMeta setOrFindTypeMeta(const ENTT_ID_TYPE id, const TypeMeta& data);

#define ECS_LABEL(__name__) \
  using __name__ = entt::tag<#__name__ ## _hs>

template<typename T>
struct TypeMetaRegistrator {};

#ifndef STRINGIFY_VA_ARG
#error "STRINGIFY_VA_ARG not defined"
#endif

#define ECS_DEFINE_METATYPE(...) \
  namespace ECS { \
  bool TypeMetaRegistrator<__VA_ARGS__>::isInit = \
    ECS::setOrFindTypeMeta( \
        entt::type_info<__VA_ARGS__>::id() \
        , TypeMeta{ STRINGIFY_VA_ARG(__VA_ARGS__) }).name != ""; \
  } /* namespace ECS */

#define ECS_DEFINE_METATYPE_TEMPLATE(...) \
  namespace ECS { \
  template<> \
  bool TypeMetaRegistrator<__VA_ARGS__>::isInit = \
    ECS::setOrFindTypeMeta( \
        entt::type_info<__VA_ARGS__>::id() \
        , TypeMeta{ STRINGIFY_VA_ARG(__VA_ARGS__) }).name != ""; \
  } /* namespace ECS */

#define ECS_DECLARE_METATYPE(TAG_NAME) \
  namespace ECS { \
  /* Used to call once per type `ECS::setTypeMeta` */ \
  /* Approach inspired by `Q_DECLARE_METATYPE` */ \
  template<> \
  struct TypeMetaRegistrator<TAG_NAME> \
  { \
    static ENTT_ID_TYPE id() { return entt::type_info<TAG_NAME>::id();} \
    static std::string name() { return std::string{entt::type_info<TAG_NAME>::name()};} \
    static bool isInit; \
  }; \
  } /* namespace ECS */

#define ECS_DECLARE_METATYPE_TEMPLATE_1ARG(TAG_NAME) \
  namespace ECS { \
  /* Used to call once per type `ECS::setTypeMeta` */ \
  /* Approach inspired by `Q_DECLARE_METATYPE_TEMPLATE_1ARG` */ \
  template<typename T1> \
  struct TypeMetaRegistrator<TAG_NAME<T1>> \
  { \
    static ENTT_ID_TYPE id() { return entt::type_info<TAG_NAME<T1>>::id();} \
    static std::string name() { return std::string{entt::type_info<TAG_NAME<T1>>::name()};} \
    static bool isInit; \
  }; \
  } /* namespace ECS */

#define ECS_DECLARE_METATYPE_TEMPLATE_2ARG(TAG_NAME) \
  namespace ECS { \
  /* Used to call once per type `ECS::setTypeMeta` */ \
  /* Approach inspired by `Q_DECLARE_METATYPE_TEMPLATE_2ARG` */ \
  template<typename T1, typename T2> \
  struct TypeMetaRegistrator<TAG_NAME<T1, T2>> \
  { \
    static ENTT_ID_TYPE id() { return entt::type_info<TAG_NAME<T1, T2>>::id();} \
    static std::string name() { return std::string{entt::type_info<TAG_NAME<T1, T2>>::name()};} \
    static bool isInit; \
  }; \
  } /* namespace ECS */

#define CREATE_ECS_TAG(TAG_NAME) \
  ECS_LABEL(TAG_NAME);

#define CREATE_ECS_COMPONENT(COMPONENT_NAME) \
  struct COMPONENT_NAME

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

#define DCHECK_ECS_COMPONENT(...) \
  DCHECK(!ECS::setOrFindTypeMeta(STRINGIFY_VA_ARG(__VA_ARGS__), ECS::TypeMeta{}).name.empty());

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
