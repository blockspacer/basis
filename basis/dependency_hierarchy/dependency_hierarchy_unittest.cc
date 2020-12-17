// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include "tests_common.h"

#include "basis/dependency_hierarchy/dependency_hierarchy.hpp"
#include "basis/files/scoped_temp_file.hpp"

#include <base/strings/strcat.h>
#include <base/strings/string_number_conversions.h>
#include <base/test/gtest_util.h>
#include <base/test/bind_test_util.h>

#include <stdio.h>

#include <algorithm>

namespace basis {

using namespace basis::dependency_error_space;
using namespace basis::error;

TEST(DependencyHierarchy, Flatten) {
  std::map<scoped_refptr<Dependency>, std::string> dependencyNames;

  // Given dependency hierarchy:
  // A -> B -> D
  //      B -> C -> D
  //                D -> F
  //      B -> E -> C
  // flatten(A) = [A,B,E,C,D,F]
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyA] = "A";

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyB] = "B";

  scoped_refptr<Dependency> dependencyC
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyC] = "C";

  scoped_refptr<Dependency> dependencyD
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyD] = "D";

  scoped_refptr<Dependency> dependencyE
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyE] = "E";

  scoped_refptr<Dependency> dependencyF
    = ::base::MakeRefCounted<Dependency>();
  dependencyNames[dependencyF] = "F";

  // A -> B -> D
  EXPECT_OK(dependencyA->addDependency(dependencyB));
  EXPECT_OK(dependencyB->addDependency(dependencyD));

  // B -> C -> D
  EXPECT_OK(dependencyB->addDependency(dependencyC));
  EXPECT_OK(dependencyC->addDependency(dependencyD));

  // B -> E -> C
  EXPECT_OK(dependencyB->addDependency(dependencyE));
  EXPECT_OK(dependencyE->addDependency(dependencyC));

  // D -> F
  EXPECT_OK(dependencyD->addDependency(dependencyF));

  auto mapDependencyNames = [&](const std::vector<scoped_refptr<Dependency>>& vec) {
    std::vector<std::string> result;
    for(scoped_refptr<Dependency> dep: vec) {
      result.push_back(dependencyNames[dep]);
    }
    return result;
  };

  // flatten(A) = [A,B,E,C,D,F]
  EXPECT_EQ(mapDependencyNames(dependencyA->flatten()), mapDependencyNames(std::vector<scoped_refptr<Dependency>>{
    dependencyA,
    dependencyB,
    dependencyE,
    dependencyC,
    dependencyD,
    dependencyF,
  }));
}

TEST(DependencyHierarchy, AddDependencyTwice) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependencies> dependencies1
    = ::base::MakeRefCounted<Dependencies>();

  EXPECT_EQ(dependencies1->size(), 0u);
  EXPECT_OK(dependencies1->addDependency(dependencyA));
  EXPECT_EQ(dependencies1->size(), 1u);

  // does nothing
  EXPECT_OK(dependencies1->addDependency(dependencyA));

  // can add dependency only once
  EXPECT_EQ(dependencies1->size(), 1u);
}

TEST(DependencyHierarchy, NullptrDepends) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_DCHECK_DEATH({ ignore_result(dependencyA->hasNestedDependency(nullptr)); });
  EXPECT_DCHECK_DEATH({ ignore_result(dependencyA->addDependency(nullptr)); });
}

TEST(DependencyHierarchy, AddSelfDependency) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyA));
  EXPECT_EQ(dependencyA->dependencies()->size(), 0u);

  EXPECT_ERROR_CODE(ERR_CIRCULAR_DEPENDENCY, dependencyA->addDependency(dependencyA));
  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyA));
  EXPECT_EQ(dependencyA->dependencies()->size(), 0u);
}

TEST(DependencyHierarchy, RemoveSelfDependency) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyA));
  EXPECT_EQ(dependencyA->dependencies()->size(), 0u);

  EXPECT_DEATH(dependencyA->removeDependency(dependencyA), "Can not remove self from dependencies");
  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyA));
  EXPECT_EQ(dependencyA->dependencies()->size(), 0u);
}

TEST(DependencyHierarchy, AddRemoveDependencies) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependencies> dependencies1
    = ::base::MakeRefCounted<Dependencies>();

  EXPECT_OK(dependencies1->addDependency(dependencyA));
  EXPECT_OK(dependencies1->addDependency(dependencyB));

  scoped_refptr<Dependency> dependencyAll
    = ::base::MakeRefCounted<Dependency>();

  ignore_result(dependencyAll->removeDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 0u);

  ignore_result(dependencyAll->addDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 2u);
  ignore_result(dependencyAll->addDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 2u);

  ignore_result(dependencyAll->removeDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 0u);
  ignore_result(dependencyAll->removeDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 0u);

  ignore_result(dependencyAll->addDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 2u);
  ignore_result(dependencyAll->removeDependencies(dependencies1));
  EXPECT_EQ(dependencyAll->dependencies()->size(), 0u);
}

// A -> B -> C
// D -> B -> C
// A -> D
TEST(DependencyHierarchy, SimpleDependencyHierarchy) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyC
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyD
    = ::base::MakeRefCounted<Dependency>();
}

// A -> B -> A
TEST(DependencyHierarchy, SimpleCircularDependency) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyB));
  EXPECT_OK(dependencyA->addDependency(dependencyB));
  EXPECT_TRUE(dependencyA->hasNestedDependency(dependencyB));
  EXPECT_FALSE(dependencyA->hasNestedDependency(dependencyA));

  EXPECT_ERROR_CODE(ERR_CIRCULAR_DEPENDENCY, dependencyB->addDependency(dependencyA));
  EXPECT_FALSE(dependencyB->hasNestedDependency(dependencyA));
}

// A -> B -> C -> A
TEST(DependencyHierarchy, CircularDependency) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyC
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_OK(dependencyA->addDependency(dependencyB));
  EXPECT_TRUE(dependencyA->hasNestedDependency(dependencyB));

  EXPECT_OK(dependencyB->addDependency(dependencyC));
  EXPECT_TRUE(dependencyB->hasNestedDependency(dependencyC));

  EXPECT_ERROR_CODE(ERR_CIRCULAR_DEPENDENCY, dependencyC->addDependency(dependencyA));
  EXPECT_FALSE(dependencyC->hasNestedDependency(dependencyA));
}

TEST(DependencyHierarchy, AllTest) {
  scoped_refptr<Dependency> dependencyA
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependency> dependencyB
    = ::base::MakeRefCounted<Dependency>();

  scoped_refptr<Dependencies> dependencies1
    = ::base::MakeRefCounted<Dependencies>();

  EXPECT_OK(dependencies1->addDependency(dependencyA));
  EXPECT_OK(dependencies1->addDependency(dependencyB));

  scoped_refptr<Dependency> dependencyC
    = ::base::MakeRefCounted<Dependency>();

  EXPECT_OK(dependencies1->addDependencies(dependencies1));

  scoped_refptr<Dependencies> dependencies2
    = ::base::MakeRefCounted<Dependencies>();

  EXPECT_OK(dependencies2->addDependency(dependencyA));
  EXPECT_OK(dependencies2->addDependency(dependencyB));
  EXPECT_OK(dependencies2->addDependency(dependencyC));

  scoped_refptr<Dependencies> dependencies3
    = ::base::MakeRefCounted<Dependencies>();

  EXPECT_OK(dependencies3->addDependency(dependencyA));
  EXPECT_OK(dependencies3->addDependency(dependencyB));
  ignore_result(dependencies2->addDependencies(dependencies1));
  ignore_result(dependencies2->addDependencies(dependencies2));
}

} // basis
