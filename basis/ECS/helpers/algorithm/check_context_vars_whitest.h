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
//
// DCHECK_CONTEXT_VARS_WHITELIST(
//   REFERENCED(ctx)
//   , ECS::include<
//       ::base::Optional<DetectChannel>
//       , ::base::Optional<WsChannel>
//       , ::base::Optional<HttpChannel>
//       , Listener::StrandComponent
//     >);
#if DCHECK_IS_ON()
#define DCHECK_CONTEXT_VARS_WHITELIST(...) \
  ::ECS::checkContextVarsWhitest(__VA_ARGS__)
#else
  #define DCHECK_CONTEXT_VARS_WHITELIST(...) \
    (void)(0)
#endif

namespace ECS {

/// \note in DEBUG builds uses DCHECK(false),
/// but in release builds returns num. of context vars. that failed check.
//
/// \note iterates all context vars. types, so avoid in release builds
/// for performance reasons, see `DCHECK_CONTEXT_VARS_WHITELIST`
template<
  class... Include
>
static int checkContextVarsWhitest(
  ECS::UnsafeTypeContext& ctx
  , ECS::include_t<Include...> = {})
{
  int count = 0;

  std::vector<ECS::UnsafeTypeContext::variable_data>& ctxVars
    = ctx.vars();

  static std::vector<
    ENTT_ID_TYPE
  > includedVec{
    {
      entt::type_info<Include>::id()...
    }
  };

  for(auto pos = ctxVars.size(); pos; --pos)
  {
    auto type_id = ctxVars[pos-1].type_id;

    const auto it
      = std::find_if(includedVec.begin()
        , includedVec.end()
        // custom comparator
        , [&type_id]
        (const ENTT_ID_TYPE& id)
        {
          DVLOG_LOC_IF(FROM_HERE, 99, ENABLE_PER_FILE_VERBOSE_MODE)
            << " x.id = "
            << id
            << " type_id ="
            << type_id;
          /// \note compare only by id, without debug name
          return id == type_id;
        }
      );

    if(it == includedVec.end())
    {
      DCHECK(false)
        << "NOT allowed to contain context var with id = "
        << type_id;
      count++;
    } else {
      DVLOG_LOC_IF(FROM_HERE, 99, ENABLE_PER_FILE_VERBOSE_MODE)
        << " context var with name ="
        << ECS::setOrFindTypeMeta(type_id, ECS::TypeMeta{}).name
        << " context var with id = "
        << type_id;
    }
  }

  return count;
}

} // namespace ECS

#undef ENABLE_PER_FILE_VERBOSE_MODE
