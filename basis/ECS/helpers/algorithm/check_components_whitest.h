#pragma once

#include <base/logging.h>

#include <basis/ECS/ecs.h>

#include <basic/log/logger.h>

#include <vector>

#ifdef ENABLE_PER_FILE_VERBOSE_MODE
#error "ENABLE_PER_FILE_VERBOSE_MODE already defined"
#endif // ENABLE_PER_FILE_VERBOSE_MODE

// extra log for development,
// can provide extra log verbosity in current file only
#define ENABLE_PER_FILE_VERBOSE_MODE 0

// USAGE
// DCHECK_COMPONENT_WHITELIST(
//   REFERENCED(registry)
//   , entityId
//   , ECS::include<
//       ECS::TcpConnection
//       , ::base::Optional<DetectChannel::SSLDetectResult>
//       , ECS::UnusedSSLDetectResultTag
//       , ::base::Optional<Listener::AcceptConnectionResult>
//       , ECS::UnusedAcceptResultTag
//     >);
//
#if DCHECK_IS_ON()
#define DCHECK_COMPONENT_WHITELIST(...) \
  ::ECS::checkComponentsWhitest(__VA_ARGS__)
#else
  #define DCHECK_COMPONENT_WHITELIST(...) \
    (void)(0)
#endif

namespace ECS {

/// \note in DEBUG builds uses DCHECK(false),
/// but in release builds returns num. of components that failed check.
//
/// \note iterates all component types, so avoid in release builds
/// for performance reasons, see `DCHECK_COMPONENT_WHITELIST`
template<
  class... Include
>
static int checkComponentsWhitest(
  ECS::Registry& registry
  , ECS::Entity entityId
  , ECS::include_t<Include...> = {})
{
  int count = 0;

  static std::vector<
    ENTT_ID_TYPE
  > includedVec{
    {
      entt::type_info<Include>::id()...
    }
  };

  // `registry.visit` - get all components of entity
  registry.visit(entityId, [&entityId, &count](const ENTT_ID_TYPE type_id)
  {
    const auto it
      = std::find_if(includedVec.begin()
        , includedVec.end()
        // custom comparator
        , [&type_id, &entityId]
        (const ENTT_ID_TYPE& id)
        {
          DVLOG_LOC_IF(FROM_HERE, 99, ENABLE_PER_FILE_VERBOSE_MODE)
            << " registry.visit "
            << " id = "
            << id
            << " type_id ="
            << type_id
            << " entityId = "
            << entityId
            << " component name = "
            << ECS::setOrFindTypeMeta(type_id, ECS::TypeMeta{}).name;
          /// \note compare only by id, without debug name
          return id == type_id;
        }
      );

    if(it == includedVec.end())
    {
      DCHECK(false)
        << " cached entity with id = "
        << entityId
        << " is NOT allowed to contain component with id = "
        << type_id
        << " component name = "
        << ECS::setOrFindTypeMeta(type_id, ECS::TypeMeta{}).name;
      count++;
    } else {
      DVLOG_LOC_IF(FROM_HERE, 99, ENABLE_PER_FILE_VERBOSE_MODE)
        << " cached entity with id = "
        << entityId
        << " re-uses component with name ="
        << ECS::setOrFindTypeMeta(type_id, ECS::TypeMeta{}).name
        << " component with id = "
        << type_id;
    }
  });

  return count;
}

} // namespace ECS

#undef ENABLE_PER_FILE_VERBOSE_MODE
