#include <base/optional.h>
#include <base/time/time.h>
#include <base/macros.h>
#include <base/stl_util.h>
#include "base/strings/string_util.h"
#include <base/command_line.h>
#include <base/logging.h>
#include <base/callback.h>
#include <base/feature_list.h>
#include <base/bind.h>
#include <base/run_loop.h>

#include <basic/macros.h>
#include <basic/bind/bind_to_task_runner.h>

#include <basis/ECS/tags.h>
#include <basis/ECS/helpers/relationship/prepend_child_entity.h>
#include <basis/ECS/helpers/relationship/foreach_top_level_child.h>
#include <basis/ECS/helpers/relationship/view_top_level_children.h>
#include <basis/ECS/helpers/relationship/remove_top_level_children_from_view.h>
#include <basis/ECS/helpers/relationship/remove_child_from_top_level.h>
#include <basis/ECS/helpers/relationship/has_child_at_top_level.h>

#include "testing/gtest/include/gtest/gtest.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

CREATE_ECS_TAG(Internal_hasChildAtTopLevelTag);
ECS_DECLARE_METATYPE(Internal_hasChildAtTopLevelTag);
ECS_DEFINE_METATYPE(Internal_hasChildAtTopLevelTag)

class TestTypeTag{};

using TagType = TestTypeTag;

ECS_DEFINE_METATYPE_TEMPLATE(ECS::ChildSiblings<TagType>);
ECS_DEFINE_METATYPE_TEMPLATE(ECS::TopLevelChildrenCount<TagType, size_t>);
ECS_DEFINE_METATYPE_TEMPLATE(ECS::ParentEntity<TagType>);
ECS_DEFINE_METATYPE_TEMPLATE(ECS::FirstChildInLinkedList<TagType>);

TEST(ECSChildrenTest, test_hierarchies_in_ECS_model)
{
  using FirstChildComponent = ECS::FirstChildInLinkedList<TagType>;
  using ChildrenComponent = ECS::ChildSiblings<TagType>;
  /// \note we assume that size of all children can be stored in `size_t`
  using ChildrenSizeComponent = ECS::TopLevelChildrenCount<TagType, size_t>;
  using ParentComponent = ECS::ParentEntity<TagType>;

  ECS::Registry registry;

  ECS::Entity parentId = registry.create();

  ECS::Entity childId = registry.create();

  DCHECK(!hasChildComponents<TagType>(registry, parentId));
  DCHECK(!hasParentComponents<TagType>(registry, childId));
  DCHECK(!hasChildComponents<TagType>(registry, childId));
  DCHECK(!hasParentComponents<TagType>(registry, parentId));

  {
    ECS::prependChildEntity<
        TagType
    >(
      REFERENCED(registry)
      , parentId
      , childId
    );

    DCHECK(hasChildComponents<TagType>(registry, childId));
    DCHECK(hasParentComponents<TagType>(registry, parentId));
    DCHECK(!hasChildComponents<TagType>(registry, parentId));
    DCHECK(!hasParentComponents<TagType>(registry, childId));

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 1);

    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, registry.create()));

    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).nextId, ECS::NULL_ENTITY);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).prevId, ECS::NULL_ENTITY);
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 1);
    DCHECK_EQ(iteratedEntities[0], childId);
  }

  ECS::Entity childTwoId = registry.create();

  {
    ECS::prependChildEntity<
      TagType
    >(
      REFERENCED(registry)
      , parentId
      , childTwoId
    );

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childTwoId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 2);

    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childId));
    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childTwoId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, registry.create()));

    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).nextId, ECS::NULL_ENTITY);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).prevId, childTwoId);

    DCHECK_EQ(registry.get<ParentComponent>(childTwoId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childTwoId).nextId, childId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childTwoId).prevId, ECS::NULL_ENTITY);
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 2);
    DCHECK_EQ(iteratedEntities[0], childTwoId);
    DCHECK_EQ(iteratedEntities[1], childId);
  }

  ECS::Entity childThreeId = registry.create();

  {
    ECS::prependChildEntity<
      TagType
    >(
      REFERENCED(registry)
      , parentId
      , childThreeId
    );

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childThreeId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 3);

    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childId));
    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childTwoId));
    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childThreeId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, registry.create()));

    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).nextId, ECS::NULL_ENTITY);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).prevId, childTwoId);

    DCHECK_EQ(registry.get<ParentComponent>(childTwoId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childTwoId).nextId, childId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childTwoId).prevId, childThreeId);

    DCHECK_EQ(registry.get<ParentComponent>(childThreeId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childThreeId).nextId, childTwoId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childThreeId).prevId, ECS::NULL_ENTITY);
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 3);
    DCHECK_EQ(iteratedEntities[0], childThreeId);
    DCHECK_EQ(iteratedEntities[1], childTwoId);
    DCHECK_EQ(iteratedEntities[2], childId);
  }

  {
    bool removeOk
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , childTwoId
      );
    DCHECK(removeOk);

    DCHECK(!registry.has<ParentComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childThreeId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 2);

    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childId));
    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childThreeId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, childTwoId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, registry.create()));

    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).nextId, ECS::NULL_ENTITY);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).prevId, childThreeId);

    DCHECK_EQ(registry.get<ParentComponent>(childThreeId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childThreeId).nextId, childId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childThreeId).prevId, ECS::NULL_ENTITY);
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 2);
    DCHECK_EQ(iteratedEntities[0], childThreeId);
    DCHECK_EQ(iteratedEntities[1], childId);
  }

  {
    bool removeOk
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , childTwoId
      );
    DCHECK(!removeOk);
  }

  {
    ECS::Entity tmpId = registry.create();

    bool removeOk
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , tmpId
      );
    DCHECK(!removeOk);
  }

  {
    bool removeOk
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , childThreeId
      );
    DCHECK(removeOk);

    DCHECK(!registry.has<ParentComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));

    DCHECK(!registry.has<ParentComponent>(childThreeId));
    DCHECK(!registry.has<ChildrenComponent>(childThreeId));
    DCHECK(!registry.has<ChildrenComponent>(childThreeId));

    DCHECK(hasChildAtTopLevel<TagType>(registry, parentId, childId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, childTwoId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, childThreeId));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, registry.create()));

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 1);

    DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).nextId, ECS::NULL_ENTITY);
    DCHECK_EQ(registry.get<ChildrenComponent>(childId).prevId, ECS::NULL_ENTITY);
  }

  {
    ECS::Entity tmpParentId = registry.create();
    ECS::Entity tmpChildId = registry.create();

    bool removeOk1
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , tmpParentId
        , tmpChildId
      );
    DCHECK(!removeOk1);

    bool removeOk2
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , tmpParentId
        , childId
      );
    DCHECK(!removeOk2);

    ECS::prependChildEntity<
      TagType
    >(
      REFERENCED(registry)
      , tmpParentId
      , tmpChildId
    );

    DCHECK_EQ(registry.get<FirstChildComponent>(tmpParentId).firstId, tmpChildId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(tmpParentId).size, 1);

    bool removeOk3
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , tmpChildId
      );
    DCHECK(!removeOk3);
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 1);
    DCHECK_EQ(iteratedEntities[0], childId);
  }

  {
    bool removeOk
      = ECS::removeChildFromTopLevel<
        TagType
      >(
        REFERENCED(registry)
        , parentId
        , childId
      );
    DCHECK(removeOk);

    DCHECK(!registry.has<ParentComponent>(childId));
    DCHECK(!registry.has<ChildrenComponent>(childId));
    DCHECK(!registry.has<ChildrenComponent>(childId));

    DCHECK(!registry.has<ParentComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));
    DCHECK(!registry.has<ChildrenComponent>(childTwoId));

    DCHECK(!registry.has<ParentComponent>(childThreeId));
    DCHECK(!registry.has<ChildrenComponent>(childThreeId));
    DCHECK(!registry.has<ChildrenComponent>(childThreeId));

    DCHECK(!registry.has<FirstChildComponent>(parentId));
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));
  }

  {
    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 0);
  }

  {
    DCHECK(!hasChildAtTopLevel<TagType>(registry, parentId, ECS::NULL_ENTITY));
    DCHECK(!hasChildAtTopLevel<TagType>(registry, ECS::NULL_ENTITY, childId));
    DCHECK(!hasChildComponents<TagType>(registry, ECS::NULL_ENTITY));
    DCHECK(!hasParentComponents<TagType>(registry, ECS::NULL_ENTITY));
  }

  {
    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));

    ECS::prependChildEntity<
      TagType
    >(
      REFERENCED(registry)
      , parentId
      , childThreeId
    );

    DCHECK_EQ(registry.get<FirstChildComponent>(parentId).firstId, childThreeId);
    DCHECK_EQ(registry.get<ChildrenSizeComponent>(parentId).size, 1);

    registry.emplace<Internal_hasChildAtTopLevelTag>(parentId);

    removeTopLevelChildrenFromView<
      TagType
    >(
      REFERENCED(registry)
      , ECS::include<
          Internal_hasChildAtTopLevelTag
        >
      , ECS::exclude<>
    );

    std::vector<ECS::Entity> iteratedEntities{};

    ECS::foreachTopLevelChild<TagType>(
      REFERENCED(registry)
      , parentId
      , ::base::BindRepeating(
        [
        ](
          std::vector<ECS::Entity>& iterated
          , ECS::Registry& registry
          , ECS::Entity parentId
          , ECS::Entity childId
        ){
          DCHECK_EQ(registry.get<ParentComponent>(childId).parentId, parentId);

          iterated.push_back(childId);
        }
        , REFERENCED(iteratedEntities)
      )
    );

    DCHECK_EQ(iteratedEntities.size(), 0);

    DCHECK(!registry.has<ChildrenSizeComponent>(parentId));
  }
}
