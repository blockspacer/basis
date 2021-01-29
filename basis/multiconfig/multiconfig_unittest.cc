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
#include "base/strings/substitute.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_environment_variable_override.h"
#include "base/environment.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"

namespace basis {

constexpr char kDefaultTestGroup[] = "test_group_1";
constexpr char kUnknownKey[] = "unknown_key";
constexpr char kDefaultKey[] = "default_key";
constexpr char kTestKeyA[] = "test_key_a";
constexpr char kTestKeyB[] = "test_key_b";
constexpr char kTestKeyC[] = "test_key_c";
constexpr char kTestKeyD[] = "test_key_d";
constexpr char kTestKeyF[] = "test_key_f";
constexpr char kResultForDefaultKey[] = "result_for_default_key";
constexpr char kResultForTestKeyA[] = "result_for_test_key_a";
constexpr char kResultForTestKeyB[] = "result_for_test_key_b";
constexpr char kResultForTestKeyC[] = "result_for_test_key_c";
constexpr char kResultForTestKeyD[] = "result_for_test_key_d";
constexpr char kResultForTestKeyF[] = "result_for_test_key_f";

static void assertClearedJsonConfEquals(
  const std::string& json_data)
{
  ASSERT_OK(JsonMultiConf::GetInstance().clearAndParseFromString(json_data));

  std::string serialized
    = JsonMultiConf::GetInstance().serializeCachedConfig();

  std::string clean_serialized;
  ASSERT_TRUE(base::ReplaceChars(serialized, " \n\r",
                             "", &clean_serialized));

  std::string clean_json_data;
  ASSERT_TRUE(base::ReplaceChars(json_data, " \n\r",
                             "", &clean_json_data));

  ASSERT_EQ(clean_serialized, clean_json_data);
}

struct TestMultiConf_1 {
  static basis::StatusOr<std::string> tryLoadString(
    const std::string& key, const std::string& configuration_group);

  // id for debug purposes
  static constexpr char kId[] = "TestMultiConf_1";
};

// static
basis::StatusOr<std::string> TestMultiConf_1::tryLoadString(
  const std::string& key, const std::string& configuration_group)
{
  DCHECK(!key.empty());

  if(base::ToLowerASCII(formatConfigNameAndGroup(key, configuration_group))
      == formatConfigNameAndGroup(kTestKeyA, configuration_group))
  {
    return std::string{kResultForTestKeyA};
  }

  if(base::ToLowerASCII(formatConfigNameAndGroup(key, configuration_group))
      == formatConfigNameAndGroup(kTestKeyB, configuration_group))
  {
    return std::string{kResultForTestKeyB};
  }

  RETURN_ERROR()
    << FROM_HERE.ToString()
    << " unable to find env. key: "
    << formatConfigNameAndGroup(key, configuration_group)
    << " in loader "
    << kId;
}

#define TEST_MULTICONF_LOADER_1 \
    basis::MultiConfLoader{ \
      ::basis::TestMultiConf_1::kId \
      , base::BindRepeating(&::basis::TestMultiConf_1::tryLoadString) \
    }

struct TestMultiConf_2 {
  static basis::StatusOr<std::string> tryLoadString(
    const std::string& key, const std::string& configuration_group);

  // id for debug purposes
  static constexpr char kId[] = "TestMultiConf_2";
};

// static
basis::StatusOr<std::string> TestMultiConf_2::tryLoadString(
  const std::string& key, const std::string& configuration_group)
{
  DCHECK(!key.empty());

  if(base::ToLowerASCII(formatConfigNameAndGroup(key, configuration_group))
       == formatConfigNameAndGroup(kTestKeyC, configuration_group))
  {
    return std::string{kResultForTestKeyC};
  }

  RETURN_ERROR()
    << FROM_HERE.ToString()
    << " unable to find env. key: "
    << formatConfigNameAndGroup(key, configuration_group)
    << " in loader "
    << kId;
}

#define TEST_MULTICONF_LOADER_2 \
    basis::MultiConfLoader{ \
      ::basis::TestMultiConf_2::kId \
      , base::BindRepeating(&::basis::TestMultiConf_2::tryLoadString) \
    }

class MultiConfTestObserver : public MultiConf::Observer {
 public:
  MultiConfTestObserver() : num_cache_changed_(0), num_option_changed_(0) {}

  ~MultiConfTestObserver() override = default;

  void onOptionReloaded(
    const MultiConfOption& option
    , const std::string& prev_value
    , const std::string& new_value) override
  {
    DVLOG(999)
      << "Detected change in configuration option from "
      << prev_value
      << " to "
      << new_value;
    ++num_option_changed_;
  }

  void onCacheReloaded() override
  {
    DVLOG(999)
      << "Detected configuration cache reload";
    ++num_cache_changed_;
  }

  std::string id() override {
    return "MultiConfTestObserver";
  }

  int num_option_changed() const { return num_option_changed_; }

  int num_cache_changed() const { return num_cache_changed_; }

 private:
  int num_cache_changed_;
  int num_option_changed_;

  DISALLOW_COPY_AND_ASSIGN(MultiConfTestObserver);
};

class MultiConfTest : public testing::Test {
  void SetUp() override {
    observer_ = std::make_unique<MultiConfTestObserver>();
    basis::MultiConf::GetInstance().AssertObserversEmpty();
    basis::MultiConf::GetInstance().addObserver(observer_.get());
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(basis::MultiConf::GetInstance().isCacheEmpty());
    ASSERT_FALSE(basis::MultiConf::GetInstance().hasOptions());
    ASSERT_OK(basis::MultiConf::GetInstance().init());
  }
  void TearDown() override {
    basis::MultiConf::GetInstance().clearCache();
    basis::MultiConf::GetInstance().clearOptions();
    basis::MultiConf::GetInstance().removeObserver(observer_.get());
    base::RunLoop().RunUntilIdle();
    basis::MultiConf::GetInstance().AssertObserversEmpty();
  }
 public:
  ::base::test::ScopedTaskEnvironment task_environment_;
  /// \note observer uses `PostTask`, so it requires `base::RunLoop().RunUntilIdle();`
  std::unique_ptr<MultiConfTestObserver> observer_;
};

TEST_F(MultiConfTest, DefaultValueTest) {
  ASSERT_TRUE(basis::MultiConf::GetInstance().isCacheEmpty());
  ASSERT_FALSE(basis::MultiConf::GetInstance().hasOptions());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(observer_->num_option_changed(), 0);
  // Skips `onCacheReloaded` because no configuration options provided.
  ASSERT_EQ(observer_->num_cache_changed(), 1);

  ScopedMultiConfObserver<std::string> tmp(
    kDefaultKey, kResultForDefaultKey, {TEST_MULTICONF_LOADER_1}, kDefaultTestGroup);
  ScopedMultiConfObserver<std::string> keyDefaultObserver
    = base::rvalue_cast(tmp);
  ASSERT_NOT_OK(TestMultiConf_1::tryLoadString(kDefaultKey, kDefaultTestGroup));

  // already added by ScopedMultiConfObserver
  {
    // must call `removeObserver(&tmp)` and `addObserver(&keyDefaultObserver)`
    ASSERT_TRUE(!tmp.is_auto_registered());
    ASSERT_TRUE(keyDefaultObserver.is_auto_registered());
    EXPECT_EQ(kResultForDefaultKey, keyDefaultObserver.GetValue());

    ASSERT_NOT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
      kDefaultKey
      , kResultForDefaultKey
      , {TEST_MULTICONF_LOADER_1}
      , kDefaultTestGroup
    }));
  }

  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kDefaultKey, kDefaultTestGroup));

  MULTICONF_String(my_conf_key_1, "abcd", {TEST_MULTICONF_LOADER_1}, kDefaultTestGroup);
  ASSERT_NOT_OK(TestMultiConf_1::tryLoadString("my_conf_key_1", kDefaultTestGroup));

  ASSERT_OK(basis::MultiConf::GetInstance().clearAndReload());
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(basis::MultiConf::GetInstance().countOptions(), 2);
  ASSERT_EQ(observer_->num_option_changed(), 2);
  ASSERT_EQ(observer_->num_cache_changed(), 2);

  {
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName("my_conf_key_1", kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache("my_conf_key_1", kDefaultTestGroup))
      , "abcd");
    EXPECT_EQ(my_conf_key_1.GetValue(), "abcd");
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kDefaultKey, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kDefaultKey, kDefaultTestGroup))
      , kResultForDefaultKey);
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(kResultForDefaultKey, keyDefaultObserver.GetValue());
  }

  {
    EXPECT_FALSE(basis::MultiConf::GetInstance().hasOptionWithName(kUnknownKey, kDefaultTestGroup));
    EXPECT_NOT_OK(basis::MultiConf::GetInstance().getAsStringFromCache(kUnknownKey, kDefaultTestGroup));
  }
}

TEST_F(MultiConfTest, ReloadJsonOptionTest) {
  ASSERT_TRUE(basis::MultiConf::GetInstance().isCacheEmpty());
  ASSERT_FALSE(basis::MultiConf::GetInstance().hasOptions());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(observer_->num_option_changed(), 0);
  // Skips `onCacheReloaded` because no configuration options provided.
  ASSERT_EQ(observer_->num_cache_changed(), 1);

  MULTICONF_String(my_conf_key_1, "abcd", {JSON_MULTICONF_LOADER}, kDefaultTestGroup);

  {
    ASSERT_OK(basis::MultiConf::GetInstance().reloadOptionWithName("my_conf_key_1"
      , kDefaultTestGroup // configuration_group
      , false // notify_cache_reload
      , false // clear_cache_on_error
    ));

    ASSERT_EQ(basis::MultiConf::GetInstance().countOptions(), 1);
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(observer_->num_option_changed(), 1);
    ASSERT_EQ(observer_->num_cache_changed(), 1);

    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName("my_conf_key_1", kDefaultTestGroup));
    ASSERT_NOT_OK(basis::JsonMultiConf::GetInstance().tryLoadString("my_conf_key_1", kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache("my_conf_key_1", kDefaultTestGroup)
    ), "abcd");
    EXPECT_EQ(my_conf_key_1.GetValue(), "abcd");
  }

  {
    std::string json_data = base::Substitute(R"raw(
  {"$1":"$2"}
  )raw", formatConfigNameAndGroup("my_conf_key_1", kDefaultTestGroup), "gdgdf");
    ASSERT_OK(JsonMultiConf::GetInstance().clearAndParseFromString(json_data));
    assertClearedJsonConfEquals(json_data);

    ASSERT_OK(basis::MultiConf::GetInstance().reloadOptionWithName("my_conf_key_1"
      , kDefaultTestGroup // configuration_group
      , true // notify_cache_reload
      , false // clear_cache_on_error
    ));

    ASSERT_EQ(basis::MultiConf::GetInstance().countOptions(), 1);
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(observer_->num_option_changed(), 2);
    ASSERT_EQ(observer_->num_cache_changed(), 2);

    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName("my_conf_key_1", kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::JsonMultiConf::GetInstance().tryLoadString("my_conf_key_1", kDefaultTestGroup)
    ), "gdgdf");
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache("my_conf_key_1", kDefaultTestGroup)
    ), "gdgdf");
    EXPECT_EQ(my_conf_key_1.GetValue(), "gdgdf");
  }

  {
    std::string json_data = base::Substitute(R"raw(
  {"$1":"$2"}
  )raw", formatConfigNameAndGroup("my_conf_key_1", kDefaultTestGroup), "fhhffg");
    ASSERT_OK(JsonMultiConf::GetInstance().clearAndParseFromString(json_data));
    assertClearedJsonConfEquals(json_data);

    ASSERT_OK(basis::MultiConf::GetInstance().reloadOptionWithName("my_conf_key_1"
      , kDefaultTestGroup // configuration_group
      , false // notify_cache_reload
      , false // clear_cache_on_error
    ));

    ASSERT_EQ(basis::MultiConf::GetInstance().countOptions(), 1);
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(observer_->num_option_changed(), 3);
    ASSERT_EQ(observer_->num_cache_changed(), 2);

    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName("my_conf_key_1", kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::JsonMultiConf::GetInstance().tryLoadString("my_conf_key_1", kDefaultTestGroup)
    ), "fhhffg");
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache("my_conf_key_1", kDefaultTestGroup)
    ), "fhhffg");
    EXPECT_EQ(my_conf_key_1.GetValue(), "fhhffg");
  }
}

TEST_F(MultiConfTest, SimpleTest) {
  ASSERT_TRUE(basis::MultiConf::GetInstance().isCacheEmpty());
  ASSERT_FALSE(basis::MultiConf::GetInstance().hasOptions());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(observer_->num_option_changed(), 0);
  // Skips `onCacheReloaded` because no configuration options provided.
  ASSERT_EQ(observer_->num_cache_changed(), 1);

  basis::ScopedMultiConfObserver<std::string> keyAObserver(
    kTestKeyA, "EMPTY", {TEST_MULTICONF_LOADER_1}, kDefaultTestGroup);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ("EMPTY", keyAObserver.GetValue());

  // already added by ScopedMultiConfObserver
  {
    ASSERT_TRUE(keyAObserver.is_auto_registered());
    ASSERT_NOT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
      kTestKeyA
      , base::nullopt
      , { TEST_MULTICONF_LOADER_1 }
      , kDefaultTestGroup
    }));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      TestMultiConf_1::tryLoadString(kTestKeyA, kDefaultTestGroup)), kResultForTestKeyA);
  }

  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyA, kDefaultTestGroup));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ("EMPTY", keyAObserver.GetValue());

  ASSERT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
    kTestKeyB
    , base::nullopt
    , { TEST_MULTICONF_LOADER_1 }
    , kDefaultTestGroup
  }));
  EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
    TestMultiConf_1::tryLoadString(kTestKeyB, kDefaultTestGroup)), kResultForTestKeyB);
  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyB, kDefaultTestGroup));

  ASSERT_OK(basis::MultiConf::GetInstance().clearAndReload());
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(basis::MultiConf::GetInstance().countOptions(), 2);
  ASSERT_EQ(observer_->num_option_changed(), 2);
  ASSERT_EQ(observer_->num_cache_changed(), 2);

  {
    EXPECT_FALSE(basis::MultiConf::GetInstance().hasOptionWithName(kUnknownKey, kDefaultTestGroup));
    EXPECT_NOT_OK(basis::MultiConf::GetInstance().getAsStringFromCache(kUnknownKey, kDefaultTestGroup));
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyA, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyA, kDefaultTestGroup))
      , kResultForTestKeyA);
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(kResultForTestKeyA, keyAObserver.GetValue());
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyB, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyB, kDefaultTestGroup))
      , kResultForTestKeyB);
  }

  /// \note added new option, need to reload cache
  ASSERT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
    kTestKeyC
    , base::nullopt
    , { TEST_MULTICONF_LOADER_2 }
    , kDefaultTestGroup
  }));
  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyC, kDefaultTestGroup));
  EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
    TestMultiConf_2::tryLoadString(kTestKeyC, kDefaultTestGroup)), kResultForTestKeyC);

  /// \note added new option, need to reload cache
  ASSERT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
    kTestKeyD
    , base::nullopt
    , { JSON_MULTICONF_LOADER }
    , kDefaultTestGroup
  }));
  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyD, kDefaultTestGroup));

  base::test::ScopedEnvironmentVariableOverride scoped_env1(
    formatConfigNameAndGroup(kTestKeyF, kDefaultTestGroup), kResultForTestKeyF);

  {
    ASSERT_TRUE(scoped_env1.IsOverridden());
    std::string key_value;
    ASSERT_TRUE(basis::EnvMultiConf::GetInstance().env_->GetVar(
      formatConfigNameAndGroup(kTestKeyF, kDefaultTestGroup), &key_value));
    ASSERT_EQ(key_value, kResultForTestKeyF);
  }

  /// \note added new option, need to reload cache
  ASSERT_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
    kTestKeyF
    , base::nullopt
    , { ENV_MULTICONF_LOADER }
    , kDefaultTestGroup
  }));
  EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyF, kDefaultTestGroup));

  MULTICONF_String(my_conf_key_1, "abcd", {ENV_MULTICONF_LOADER}, kDefaultTestGroup);

  base::test::ScopedEnvironmentVariableOverride scoped_env2(
    formatConfigNameAndGroup("my_conf_key_1", kDefaultTestGroup), "12345");

  {
    ASSERT_TRUE(scoped_env2.IsOverridden());
    std::string key_value;
    ASSERT_TRUE(basis::EnvMultiConf::GetInstance().env_->GetVar(
      formatConfigNameAndGroup("my_conf_key_1", kDefaultTestGroup), &key_value));
    ASSERT_EQ(key_value, "12345");
  }

  std::string json_data = base::Substitute(R"raw(
{"$1":"$2"}
)raw", formatConfigNameAndGroup(kTestKeyD, kDefaultTestGroup), kResultForTestKeyD);
  ASSERT_OK(JsonMultiConf::GetInstance().clearAndParseFromString(json_data));
  assertClearedJsonConfEquals(json_data);

  ASSERT_OK(basis::MultiConf::GetInstance().clearAndReload());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
    basis::EnvMultiConf::GetInstance().tryLoadString(kTestKeyF, kDefaultTestGroup)
  ), kResultForTestKeyF);

  EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
    basis::JsonMultiConf::GetInstance().tryLoadString(kTestKeyD, kDefaultTestGroup)
  ), kResultForTestKeyD);

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName("my_conf_key_1", kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::EnvMultiConf::GetInstance().tryLoadString("my_conf_key_1", kDefaultTestGroup)
    ), "12345");
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache("my_conf_key_1", kDefaultTestGroup)
    ), "12345");
    EXPECT_EQ(my_conf_key_1.GetValue(), "12345");
  }

  {
    EXPECT_FALSE(basis::MultiConf::GetInstance().hasOptionWithName(kUnknownKey, kDefaultTestGroup));
    EXPECT_NOT_OK(basis::MultiConf::GetInstance().getAsStringFromCache(kUnknownKey, kDefaultTestGroup));
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyA, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyA, kDefaultTestGroup))
      , kResultForTestKeyA);
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyB, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyB, kDefaultTestGroup))
      , kResultForTestKeyB);
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyC, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyC, kDefaultTestGroup))
      , kResultForTestKeyC);
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyD, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyD, kDefaultTestGroup))
      , kResultForTestKeyD);
  }

  {
    EXPECT_TRUE(basis::MultiConf::GetInstance().hasOptionWithName(kTestKeyF, kDefaultTestGroup));
    EXPECT_EQ(CHECKED_CONSUME_STATUS_VALUE(
      basis::MultiConf::GetInstance().getAsStringFromCache(kTestKeyF, kDefaultTestGroup))
      , kResultForTestKeyF);
  }
}

} // namespace basis
