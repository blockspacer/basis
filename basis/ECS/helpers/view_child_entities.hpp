#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/child_linked_list.hpp>
#include <basis/ECS/components/child_linked_list_size.hpp>
#include <basis/ECS/components/first_child_in_linked_list.hpp>
#include <basis/ECS/components/parent_entity.hpp>
#include <basis/ECS/helpers/has_parent_components.hpp>
#include <basis/ECS/helpers/is_child_of.hpp>

#include <base/logging.h>
#include <base/callback.h>
#include <base/sequence_checker.h>

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

#if DCHECK_IS_ON()
    for(const ECS::Entity& childId:
      registry_.template view<TagType>())
    {
      DCHECK(registry_.has<TagType>(childId));
    }
#endif // DCHECK_IS_ON()
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

// Iterates each entity in linked list to create view associated with them.
//
// USAGE
//
//  auto scopedView
//    = ECS::viewChildEntities<TagType>(
//      REFERENCED(registry)
//      , parentEntityId
//    );
//
//  for(const ECS::Entity& childId: scopedView.view())
//  {
//    DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);
//
//    DCHECK(parentEntityId != childId);
//
//    DCHECK_PARENT_ENTITY_COMPONENTS(parentEntityId, &registry, TagType);
//
//    // update `prev` and `next` links in `ChildLinkedList` hierarchy
//    bool isRemovedFromListLinks
//      = removeChildLinks<TagType>(
//          REFERENCED(registry)
//          , childId // childIdToRemove
//          , childId // listBeginId
//          , ECS::NULL_ENTITY // listEndId
//        );
//    DCHECK(isRemovedFromListLinks);
//
//    DCHECK(!registry.has<Internal_ChildrenToRemove>(childId));
//    registry.emplace<Internal_ChildrenToRemove>(childId);
//
//    DCHECK(registry.has<FirstChildComponent>(parentEntityId));
//    DCHECK(registry.has<ChildrenSizeComponent>(parentEntityId));
//  }
template <
  typename TagType // unique type tag for all children
>
MUST_USE_RETURN_VALUE
auto viewChildEntities(
  ECS::Registry& registry
  , ECS::Entity parentEntityId)
{
  using ChildrenComponent = ChildLinkedList<TagType>;
  using ParentComponent = ParentEntity<TagType>;
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ChildLinkedListSize<TagType, size_t>;

  CREATE_ECS_TAG(Internal_ChildrenToView);

  DCHECK(registry.view<Internal_ChildrenToView>().empty());

  ECS::foreachChildEntity<TagType>(
    REFERENCED(registry)
    , parentEntityId
    , base::BindRepeating(
      [
      ](
        ECS::Registry& registry
        , ECS::Entity parentId
        , ECS::Entity childId
      ){
        DCHECK(parentId != childId);

        DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);
        DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

        DCHECK(isChildOf<TagType>(REFERENCED(registry), parentId, childId));

        DCHECK(!registry.has<Internal_ChildrenToView>(childId));
        registry.emplace<Internal_ChildrenToView>(childId);
      }
    )
  );

  /// \note `Scoped*View` must remove component on scope exit
  return ScopedChildView<Internal_ChildrenToView>(REFERENCED(registry));
}

} // namespace ECS
