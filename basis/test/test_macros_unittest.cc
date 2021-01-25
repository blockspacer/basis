// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tests_common.h"

#include "basis/test/test_macros.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include "base/rvalue_cast.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_restrictions.h"

namespace basis {

class TestMacrosTest : public testing::Test {
 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(TestMacrosTest, ExpectElementsEq) {
  std::vector<int> vecA{1,2,3};
  EXPECT_ELEMENTS_EQ(vecA, 1, 2, 3);
}

TEST_F(TestMacrosTest, StrContains) {
  ASSERT_STR_CONTAINS("abcd", "bc");
  ASSERT_STR_NOT_CONTAINS("agfm", "bc");
}

}  // namespace basis
