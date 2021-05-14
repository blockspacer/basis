#pragma once

#include <basis/ECS/ecs.h>
#include <basis/ECS/components/relationship/child_siblings.h>
#include <basis/ECS/components/relationship/top_level_children_count.h>
#include <basis/ECS/components/relationship/first_child_in_linked_list.h>
#include <basis/ECS/components/relationship/parent_entity.h>
#include <basis/ECS/helpers/relationship/has_parent_components.h>
#include <basis/ECS/helpers/relationship/is_child_at_top_level_of.h>

#include <base/logging.h>
#include <base/callback.h>
#include <base/sequence_checker.h>
#include <base/macros.h>

#include <basic/macros.h>

namespace ECS {

/// \note `Scoped*View` removes component on scope exit
template <
  typename TagType // unique type tag for all children
>
class ScopedChildView {
public:
  ScopedChildView(ECS::Registry& registry)
    : registry_(registry)
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  ~ScopedChildView()
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    for(const ECS::Entity& childId:
      registry_.template view<TagType>())
    {
      registry_.remove<TagType>(childId);
    }
  }

  ECS::View<entt::exclude_t<>, TagType>
  view()
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    return registry_.template view<TagType>();
  }

private:
  ECS::Registry& registry_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ScopedChildView);
};

CREATE_ECS_TAG(Internal_ChildrenToView);

/// \note does not iterate hierarchy recursively
/// i.e. does not iterate children of children of children...
//
// Iterates each entity in linked list to create view associated with them.
//
// USAGE
//
//  auto scopedView
//    = ECS::viewTopLevelChildren<TagType>(
//        REFERENCED(registry)
//        , parentEntityId
//      );
//
//  for(const ECS::Entity& childId: scopedView.view())
//  {
//    DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);
//
//    DCHECK(parentEntityId != childId);
//
//    DCHECK_PARENT_ENTITY_COMPONENTS(parentEntityId, &registry, TagType);
//
//    // update `prev` and `next` links in `ChildSiblings` hierarchy
//    bool isRemovedFromListLinks
//      = removeFromSiblings<TagType>(
//          REFERENCED(registry)
//          , childId // childIdToRemove
//          , childId // listBeginId
//          , ECS::NULL_ENTITY // listEndId
//        );
//    DCHECK(isRemovedFromListLinks);
//
//    DCHECK(!registry.has<Internal_ChildrenToView>(childId));
//    registry.emplace<Internal_ChildrenToView>(childId);
//
//    DCHECK(registry.has<FirstChildComponent>(parentEntityId));
//    DCHECK(registry.has<ChildrenSizeComponent>(parentEntityId));
//  }
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
auto viewTopLevelChildren(
  ECS::Registry& registry
  , ECS::Entity parentEntityId)
{
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;

  DCHECK(registry.view<Internal_ChildrenToView>().empty());

  ECS::foreachTopLevelChild<TagType>(
    REFERENCED(registry)
    , parentEntityId
    , ::base::BindRepeating(
      [
      ](
        ECS::Registry& registry
        , ECS::Entity parentId
        , ECS::Entity childId
      ){
        DCHECK(parentId != childId);

        DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);
        DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

        DCHECK(isChildAtTopLevelOf<TagType>(REFERENCED(registry), parentId, childId));

        DCHECK(!registry.has<Internal_ChildrenToView>(childId));
        registry.emplace<Internal_ChildrenToView>(childId);
      }
    )
  );

  /// \note `Scoped*View` must remove component on scope exit
  return ScopedChildView<Internal_ChildrenToView>(REFERENCED(registry));
}

} // namespace ECS

ECS_DECLARE_METATYPE(Internal_ChildrenToView);
