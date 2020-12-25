#pragma once

#include <basis/ECS/ecs.hpp>

#include <basis/ECS/components/relationship/child_siblings.hpp>
#include <basis/ECS/components/relationship/first_child_in_linked_list.hpp>
#include <basis/ECS/components/relationship/parent_entity.hpp>
#include <basis/ECS/components/relationship/top_level_children_count.hpp>
#include <basis/ECS/helpers/relationship/foreach_top_level_child.hpp>
#include <basis/ECS/helpers/relationship/view_top_level_children.hpp>
#include <basis/ECS/helpers/relationship/remove_from_siblings.hpp>
#include <basis/ECS/helpers/relationship/remove_parent_components.hpp>
#include <basis/ECS/helpers/relationship/remove_child_components.hpp>
#include <basis/ECS/helpers/relationship/is_child_at_top_level_of.hpp>

#include <base/logging.h>

namespace ECS {

// To work around issues during iterations we store aside
// the entities and the components to be removed
// and perform the operations at the end of the iteration.
CREATE_ECS_TAG(Internal_ChildrenToRemove);

/// \todo not tested with recursive hierarchy
/// i.e. does not iterate children of children of children...
//
// Removes all children associated with `parent`.
// Does modification of components both in parent and children.
/// \note does not destroy children entities,
/// only removes them from hierarchy
//
// USAGE
//
// removeTopLevelChildrenFromView<
//   TagType
// >(
//   REFERENCED(registry)
//   , ECS::include<
//       ECS::UnusedTag
//     >
//   , ECS::exclude<
//       // entity in destruction
//       ECS::NeedToDestroyTag
//       // entity not fully created
//       , ECS::DelayedConstruction
//     >
// );
template <
  typename TagType // unique type tag for all children
  , typename... Include
  , typename... Exclude
>
void removeTopLevelChildrenFromView(
  ECS::Registry& registry
  , ECS::include_t<Include...> = {}
  , ECS::exclude_t<Exclude...> = {})
{
  using ParentComponent = ParentEntity<TagType>;
  using FirstChildComponent = FirstChildInLinkedList<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = TopLevelChildrenCount<TagType, size_t>;

  auto targetView
    = registry.view<
        FirstChildComponent
        , Include...
      >(
        entt::exclude<
          Exclude...
        >
      );

  for(const ECS::Entity& parentEntityId: targetView)
  {
    DCHECK_PARENT_ENTITY_COMPONENTS(parentEntityId, &registry, TagType);

    auto scopedView
      = ECS::viewTopLevelChildren<TagType>(
          REFERENCED(registry)
          , parentEntityId
        );

    for(const ECS::Entity& childId: scopedView.view())
    {
      DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

      DCHECK(parentEntityId != childId);

      DCHECK_PARENT_ENTITY_COMPONENTS(parentEntityId, &registry, TagType);

      DCHECK(isChildAtTopLevelOf<TagType>(REFERENCED(registry), parentEntityId, childId));

      // update `prev` and `next` links in `ChildSiblings` hierarchy
      bool isRemovedFromListLinks
        = removeFromSiblings<TagType>(
            REFERENCED(registry)
            , childId // childIdToRemove
            // because `childIdToRemove` same as `listBeginId`
            // it will take 1 iteraton to perform `removeFromSiblings`
            , childId // listBeginId
            , ECS::NULL_ENTITY // listEndId
          );
      DCHECK(isRemovedFromListLinks);

      DCHECK(!registry.has<Internal_ChildrenToRemove>(childId));
      registry.emplace<Internal_ChildrenToRemove>(childId);

      DCHECK(registry.has<FirstChildComponent>(parentEntityId));
      DCHECK(registry.has<ChildrenSizeComponent>(parentEntityId));
    } // for(const ECS::Entity& childId: scopedView.view())
  } // for(const ECS::Entity& parentEntityId: group)

  /// \note create new group to avoid iterator invalidation
  for(const ECS::Entity& childId:
    registry.view<Internal_ChildrenToRemove>())
  {
    DCHECK_CHILD_ENTITY_COMPONENTS(childId, &registry, TagType);

    // entity must be fully created
    DCHECK(!registry.has<ECS::DelayedConstruction>(childId));

    // remove all components associated with `child`
    removeChildComponents<TagType>(
      REFERENCED(registry)
      , childId
    );

    registry.remove<Internal_ChildrenToRemove>(childId);
  }

  for(const ECS::Entity& parentId
    /// \note new group due to iterator invalidation
     : targetView)
  {
    DCHECK_PARENT_ENTITY_COMPONENTS(parentId, &registry, TagType);

    // remove all components associated with `parent`
    removeParentComponents<TagType>(
      REFERENCED(registry)
      , parentId
    );

    /// \note `parentId` reference may be broken here
    /// due to components removal
    /// (same components that we used during iteration)
  }

  for(const ECS::Entity& parentId
    /// \note new group due to iterator invalidation
     : targetView)
  {
    DCHECK_ECS_ENTITY(parentId, &registry);
    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));
  }
}

} // namespace ECS

ECS_DECLARE_METATYPE(Internal_ChildrenToRemove);
