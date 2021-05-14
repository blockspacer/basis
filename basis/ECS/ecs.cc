#include <basis/ECS/ecs.h> // IWYU pragma: associated

#include <basic/macros.h>

#include <base/macros.h>
#include <base/logging.h>

namespace ECS {

TypeMeta setOrFindTypeMeta(const ENTT_ID_TYPE id, const TypeMeta& data)
{
  // Initializer Function Trick, see stackoverflow.com/a/17624722
  static std::map<
      ENTT_ID_TYPE
      , TypeMeta
    > idToName{};

  if(idToName.find(id) != idToName.end()) {
    return idToName[id];
  }

  idToName[id] = COPIED(data);

  return data;
}

} // namespace ECS
