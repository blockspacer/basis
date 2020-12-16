// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tests_common.h"

#include "basis/promise/post_task_executor.h"
#include "basis/promise/do_nothing_promise.h"

#include "base/test/gtest_util.h"
#include "base/test/bind_test_util.h"
#include "base/bind.h"
#include "basis/promise/abstract_promise.h"
#include "basis/promise/helpers.h"
#include "base/task_runner.h"

namespace base {
namespace internal {

class PostTaskExecutorTest : public testing::Test {
 public:
  template <typename CallbackT>
  WrappedPromise CreatePostTaskPromise(const Location& from_here,
                                       CallbackT&& task) {
    // Extract properties from |task| callback.
    using CallbackTraits = CallbackTraits<std::decay_t<CallbackT>>;

    internal::PromiseExecutor::Data executor_data(
        in_place_type_t<
            internal::PostTaskExecutor<typename CallbackTraits::ReturnType>>(),
        internal::ToCallbackBase(std::move(task)));

    return WrappedPromise(AbstractPromise::CreateNoPrerequisitePromise(
        from_here, RejectPolicy::kMustCatchRejection,
        internal::DependentList::ConstructUnresolved(),
        std::move(executor_data)));
  }
};

TEST_F(PostTaskExecutorTest, OnceClosure) {
  bool run = false;

  WrappedPromise p = CreatePostTaskPromise(
      FROM_HERE, BindOnce([](bool* run) { *run = true; }, &run));

  p.Execute();

  EXPECT_TRUE(run);
}

TEST_F(PostTaskExecutorTest, RepeatingClosure) {
  bool run = false;

  WrappedPromise p = CreatePostTaskPromise(
      FROM_HERE, BindRepeating([](bool* run) { *run = true; }, &run));

  p.Execute();

  EXPECT_TRUE(run);
}

TEST_F(PostTaskExecutorTest, DoNothing) {
  // Check it compiles and the executor doesn't crash when run.
  WrappedPromise p = CreatePostTaskPromise(FROM_HERE, DoNothing());

  p.Execute();
}

}  // namespace internal
}  // namespace base
