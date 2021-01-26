#include "tests_common.h"

#include "basis/multiconfig/multiconfig.hpp"

#include "basis/promise/post_task_executor.h"
#include "basis/promise/do_nothing_promise.h"

#include "base/test/gtest_util.h"
#include "base/test/bind_test_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/bind.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "base/rvalue_cast.h"
#include "basis/promise/abstract_promise.h"
#include "basis/promise/helpers.h"
#include "base/task_runner.h"

namespace basis {

// Configuration loader that uses environment vars
struct TestMultiConf {
  // Loads configuraton value from environment vars in order:
  // * key
  // * uppercase(key)
  // * lowercase(key)
  static basis::StatusOr<std::string> tryLoadString(const std::string& key);

  // id for debug purposes
  static constexpr char kId[] = "TestMultiConf";
};

constexpr char kTestKeyA[] = "test_key_a";
constexpr char kTestKeyB[] = "test_key_b";
constexpr char kResultForTestKeyA[] = "result_for_test_key_a";
constexpr char kResultForTestKeyB[] = "result_for_test_key_b";

// static
basis::StatusOr<std::string> TestMultiConf::tryLoadString(const std::string& key)
{
  DCHECK(!key.empty());

  if(base::ToLowerASCII(key) == kTestKeyA)
  {
    return std::string{kResultForTestKeyA};
  }

  if(base::ToLowerASCII(key) == kTestKeyB)
  {
    return std::string{kResultForTestKeyB};
  }

  RETURN_ERROR()
    << FROM_HERE.ToString()
    << " unable to find env. key: "
    << key;
}

#define TEST_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::TestMultiConf::kId \
      , base::BindRepeating(&::basis::TestMultiConf::tryLoadString) \
    }

class MultiConfTest : public testing::Test {
 public:
  ::base::test::ScopedTaskEnvironment task_environment_;
};

TEST_F(MultiConfTest, Simple) {
  basis::MultiConf::GetInstance().addEntry(basis::MultiConfEntry{
    kTestKeyA
    , base::nullopt
    , { TEST_MULTICONF_LOADER }
  });

  basis::MultiConf::GetInstance().addEntry(basis::MultiConfEntry{
    kTestKeyB
    , base::nullopt
    , { TEST_MULTICONF_LOADER }
  });

  EXPECT_OK(basis::MultiConf::GetInstance().init());

  {
    basis::StatusOr<std::string> value
      = basis::MultiConf::GetInstance().getAsStringFromCache("some_unknown_key");
    EXPECT_NOT_OK(value);
  }

  {
    basis::StatusOr<std::string> value
      = basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyA);
    EXPECT_OK(value);
    EXPECT_EQ(value.ValueOrDie(), kResultForTestKeyA);
  }

  {
    basis::StatusOr<std::string> value
      = basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyB);
    EXPECT_OK(value);
    EXPECT_EQ(value.ValueOrDie(), kResultForTestKeyB);
  }
}

} // namespace basis
