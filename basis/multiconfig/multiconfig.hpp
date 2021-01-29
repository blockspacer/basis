#pragma once

#include "basis/status/status_macros.hpp"

#include <base/gtest_prod_util.h>
#include <base/logging.h>
#include <base/values.h>
#include <base/callback_forward.h>
#include <base/bind.h>
#include <base/base_export.h>
#include <base/macros.h>
#include <base/no_destructor.h>
#include <base/optional.h>
#include <base/rvalue_cast.h>
#include <base/files/file_path.h>
#include <base/threading/thread_collision_warner.h>
#include <base/strings/string_number_conversions.h>
#include <base/observer_list_threadsafe.h>
#include <base/numerics/safe_conversions.h>
#include <base/numerics/floating_point_comparison.h>

#include <string>
#include <ostream>

#define ENV_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::EnvMultiConf::kId \
      , base::BindRepeating( \
          &::basis::EnvMultiConf::tryLoadString \
          , base::Unretained(&EnvMultiConf::GetInstance())) \
    }

#define CMD_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::CmdMultiConf::kId \
      , base::BindRepeating( \
          &::basis::CmdMultiConf::tryLoadString \
          , base::Unretained(&CmdMultiConf::GetInstance())) \
    }

#define JSON_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::JsonMultiConf::kId \
      , base::BindRepeating( \
          &::basis::JsonMultiConf::tryLoadString \
          , base::Unretained(&JsonMultiConf::GetInstance())) \
    }

#define BUILTIN_MULTICONF_LOADERS \
  { \
    CMD_MULTICONF_LOADER \
    , ENV_MULTICONF_LOADER \
    , JSON_MULTICONF_LOADER \
  }

/// \note Each `MULTICONF_*` macro expected to create single variable,
/// so you will be able to write code: `static MULTICONF_String(...)`.
///
/// \note You can not create same option `my_key` using
/// `MULTICONF_String(my_key, "", MY_LOADERS);`
/// multiple times (even in different files or plugins).
/// Use multiple configuration groups to avoid collision like so:
/// `MULTICONF_String(my_key, "", MY_LOADERS, CONFIG_GROUP_A);`
/// `MULTICONF_String(my_key, "", MY_LOADERS, CONFIG_GROUP_B);`

// USAGE
//
// MULTICONF_String(my_conf_key, "abcd", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_String(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<std::string> KEY_NAME \
    {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Bool(my_conf_key, "true", BUILTIN_MULTICONF_LOADERS);
// MULTICONF_Bool(my_conf_key, "True", BUILTIN_MULTICONF_LOADERS);
// MULTICONF_Bool(my_conf_key, "TRUE", BUILTIN_MULTICONF_LOADERS);
// MULTICONF_Bool(my_conf_key, "1", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Bool(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<bool> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Int(my_conf_key, "-12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Int(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<int> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Uint(my_conf_key, "12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Uint(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<unsigned> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Int64(my_conf_key, "-12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Int64(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<int64_t> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Int32(my_conf_key, "-12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Int32(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<int32_t> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Uint32(my_conf_key, "12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Uint32(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<uint32_t> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Uint64(my_conf_key, "12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Uint64(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<uint64_t> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_SizeT(my_conf_key, "12345", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_SizeT(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<size_t> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Double(my_conf_key, "1.12", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Double(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<double> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

// USAGE
//
// MULTICONF_Float(my_conf_key, "1.12", BUILTIN_MULTICONF_LOADERS);
//
#define MULTICONF_Float(KEY_NAME, DEFAULT_STR, ...) \
  basis::ScopedMultiConfObserver<float> KEY_NAME \
    = {STRINGIFY(KEY_NAME) \
      , DEFAULT_STR \
      , __VA_ARGS__}

namespace base {

class Environment;
class CommandLine;

} // namespace base

namespace basis {

std::string formatConfigNameAndGroup(
  const std::string&name, const std::string group);

// Configuration loader that uses environment vars
class JsonMultiConf {
 public:
  // Thread safe GetInstance.
  static JsonMultiConf& GetInstance();

  // Loads configuraton value from environment vars in order:
  // * key
  // * uppercase(key)
  // * lowercase(key)
  basis::StatusOr<std::string> tryLoadString(
    const std::string& name, const std::string& configuration_group);

  /// \note resets cache even in case of loading error
  MUST_USE_RETURN_VALUE
  basis::Status clearAndParseFromFilePath(
    const base::FilePath& file_path);

  /// \note resets cache even in case of loading error
  // Initializes the instance from a given JSON string,
  // returning true if the string was successfully parsed.
  MUST_USE_RETURN_VALUE
  basis::Status clearAndParseFromString(
    const std::string& json_data);

  // for test purposes
  std::string serializeCachedConfig() const;

 public:
  // id for debug purposes
  static constexpr char kId[] = "JsonMultiConf";

 private:
  JsonMultiConf();

 private:
  FRIEND_TEST_ALL_PREFIXES(MultiConfTest, SimpleTest);

  friend class base::NoDestructor<JsonMultiConf>;

  base::Optional<base::Value> cached_dictionary_;

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  DISALLOW_COPY_AND_ASSIGN(JsonMultiConf);
};

// Configuration loader that uses environment vars
class EnvMultiConf {
 public:
  // Thread safe GetInstance.
  static EnvMultiConf& GetInstance();

  // Loads configuraton value from environment vars in order:
  // * key
  // * uppercase(key)
  // * lowercase(key)
  basis::StatusOr<std::string> tryLoadString(
    const std::string& name, const std::string& configuration_group);

 public:
  // id for debug purposes
  static constexpr char kId[] = "EnvMultiConf";

 private:
  EnvMultiConf();

 private:
  FRIEND_TEST_ALL_PREFIXES(MultiConfTest, SimpleTest);

  std::unique_ptr<base::Environment> env_;

  friend class base::NoDestructor<EnvMultiConf>;

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  DISALLOW_COPY_AND_ASSIGN(EnvMultiConf);
};

// Configuration loader that uses command line switches
class CmdMultiConf {
 public:
  // Thread safe GetInstance.
  static CmdMultiConf& GetInstance();

  // Loads configuraton value from command line switches in order:
  // * key
  // * uppercase(key)
  // * lowercase(key)
  basis::StatusOr<std::string> tryLoadString(
    const std::string& name, const std::string& configuration_group);

 public:
  // id for debug purposes
  static constexpr char kId[] = "CmdMultiConf";

 private:
  CmdMultiConf();

 private:
  FRIEND_TEST_ALL_PREFIXES(MultiConfTest, SimpleTest);

  const base::CommandLine* command_line_;

  friend class base::NoDestructor<CmdMultiConf>;

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  DISALLOW_COPY_AND_ASSIGN(CmdMultiConf);
};

// Wraps functions from any configuration loader
struct MultiConfLoader {
  using LoaderFunc
    = base::RepeatingCallback<
        basis::StatusOr<std::string>(
          const std::string& name
          , const std::string& configuration_group)
      >;

  std::string name;
  LoaderFunc func;
};

// Configuration option that can be read from file, environment vars, etc.
struct MultiConfOption
{
 public:
  MultiConfOption(
    const std::string& name
    , const base::Optional<std::string>& default_value
    , const std::initializer_list<MultiConfLoader>& loaders
    , const std::string& configuration_group);

  friend bool operator<(const MultiConfOption& a, const MultiConfOption& b) {
    return formatConfigNameAndGroup(a.name, a.configuration_group)
           < formatConfigNameAndGroup(b.name, b.configuration_group);
  }

  friend std::ostream& operator<<(std::ostream& out, const MultiConfOption& i) {
    return out << formatConfigNameAndGroup(i.name, i.configuration_group);
  }

  std::string name;
  base::Optional<std::string> default_str = base::nullopt;
  std::vector<MultiConfLoader> loaders;
  std::string configuration_group;
};

class MultiConf {
 public:
  class Observer {
   public:
    Observer();

    virtual ~Observer();

    /// \note will trigger only if configuration value loaded without errors
    virtual void onOptionReloaded(
      const MultiConfOption& option
      , const std::string& prev_value
      , const std::string& new_value)  = 0;

    /// \note will NOT trigger if `known_config_options_.empty()`.
    virtual void onCacheReloaded()  = 0;

    virtual std::string id() {
      return "MultiConf::Observer";
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  ~MultiConf();

  // Thread safe GetInstance.
  static MultiConf& GetInstance();

  // Add a non owning pointer
  void addObserver(
    MultiConf::Observer* observer);

  // Does nothing if the |observer| is
  // not in the list of known observers.
  void removeObserver(
    MultiConf::Observer* observer);

  void AssertObserversEmpty() {
    DCHECK(observers_);

    /// \note thread-safe, so skip `debug_thread_collision_warner_`
    observers_->AssertEmpty();
  }

  // may be called from `clearAndReload()` or `reloadOption()`
  void notifyCacheReloaded();

  // called if `reloadOption()` succeeded
  void notifyOptionReloaded(
    const MultiConfOption& option
    , const std::string& prev_value
    , const std::string& new_value);

  MUST_USE_RETURN_VALUE
  basis::Status init() {
    DCHECK_EQ(current_config_cache_.size(), 0);
    RETURN_WITH_MESSAGE_IF_NOT_OK(clearAndReload())
      << "Failed to initialize configuration";
    RETURN_OK();
  }

  MUST_USE_RETURN_VALUE
  bool isCacheEmpty() const NO_EXCEPTION {
    return current_config_cache_.empty();
  }

  void clearCache() NO_EXCEPTION {
    current_config_cache_.clear();
  }

  MUST_USE_RETURN_VALUE
  size_t countOptions() const NO_EXCEPTION {
    return known_config_options_.size();
  }

  MUST_USE_RETURN_VALUE
  bool hasOptions() const NO_EXCEPTION {
    return !known_config_options_.empty();
  }

  void clearOptions() NO_EXCEPTION {
    known_config_options_.clear();
  }

  // Adds configuration option to `known_config_options_`
  basis::Status addOption(const MultiConfOption& option);

  /// \note Will reload cache even if nothing changed.
  /// \note Will do nothing if `known_config_options_.empty()`.
  // Updates config based on current content of config files, env. vars, etc.
  // Populates `current_config_cache_`
  basis::Status clearAndReload(
    // if your application requires all configuration values
    // to be valid or have defaults, than set `clear_cache_on_error` to true
    bool clear_cache_on_error = true);

  // Finds key with provided name in `current_config_cache_`
  basis::StatusOr<std::string> getAsStringFromCache(const std::string& name
    , const std::string& configuration_group);

  bool hasOptionWithName(const std::string& name
    , const std::string& configuration_group);

  // Populates `current_config_cache_`
  // Calls `reloadOption`
  /// \note Does not return error if configuration option has default value.
  basis::Status reloadOptionWithName(
    const std::string& name
    , const std::string& configuration_group
    // if want to reload multiple options in batches and
    // notify about change only once,
    // than set `notify_cache_reload_on_success` to true
    , bool notify_cache_reload_on_success = true
    // if your application requires all configuration values
    // to be valid or have defaults, than set `clear_cache_on_error` to true
    , bool clear_cache_on_error = true);

 private:
  MultiConf();

  // Calls each loader in `MultiConfOption` until it returns value.
  /// \note Ignores default value
  basis::StatusOr<std::string> loadAsStringWithoutDefaults(
    const MultiConfOption& option);

  // Populates `current_config_cache_`
  /// \note Does not return error if configuration option has default value.
  basis::Status reloadOption(
    const MultiConfOption& option
    , bool notify_cache_reload_on_success
    , bool clear_cache_on_error);

 private:
  friend class base::NoDestructor<MultiConf>;

  friend class MultiConf::Observer;

  /// \note ObserverListThreadSafe may be used from multiple threads
  const scoped_refptr<
      ::base::ObserverListThreadSafe<MultiConf::Observer>
    > observers_;

  std::set<MultiConfOption> known_config_options_;

  std::map<std::string, std::string> current_config_cache_;

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  DISALLOW_COPY_AND_ASSIGN(MultiConf);
};

template<typename T>
class ScopedMultiConfObserver : public MultiConf::Observer {
 public:
  // USE CASE
  // `FileA.cc` creates configuration option `my_option` using `MULTICONF_String`
  // `FileB.cc` wants to use `my_option` without need to access `FileA.h`,
  // but it can not use `MULTICONF_String` again.
  // So `FileB.cc` can create `ScopedMultiConfObserver`
  // with `auto_registered_(false)`.
  ScopedMultiConfObserver(const std::string& target_name
    , const std::string& default_name
    , const std::string& configuration_group)
    : target_name_(target_name)
    , auto_registered_(false)
  {
    basis::StatusOr<T> statusor = parseValueAs<T>(default_name);
    DCHECK(statusor.ok())
      << "default configuration value expected to be valid";
    if(statusor.ok()) {
      cached_value_ = base::rvalue_cast(statusor.ConsumeValueOrDie());
    } else {
      error_status_ = statusor.status();
    }
  }

  // Used by `MULTICONF_*` macros to both create configuration option
  // and register observer using single var.
  /// \note Automatically adds configuration option
  /// \note Automatically adds and removes itself from observer list
  ScopedMultiConfObserver(const std::string& target_name
    , const std::string& default_name
    , const std::initializer_list<MultiConfLoader>& loaders
    , const std::string& configuration_group)
    : target_name_(target_name)
    , auto_registered_(true)
  {
    CHECK_GT(loaders.size(), 0)
      << "No configuration loaders provided for option:"
      << target_name;

    CHECK_OK(basis::MultiConf::GetInstance().addOption(basis::MultiConfOption{
      target_name
      , default_name
      , loaders
      , configuration_group
    }));

    basis::StatusOr<T> statusor = parseValueAs<T>(default_name);
    DCHECK(statusor.ok())
      << "default configuration value expected to be valid";
    if(statusor.ok()) {
      cached_value_ = base::rvalue_cast(statusor.ConsumeValueOrDie());
    } else {
      error_status_ = statusor.status();
    }
    basis::MultiConf::GetInstance().addObserver(this);
  }

  ~ScopedMultiConfObserver() override {
    /// \note make sure `ScopedMultiConfObserver`
    /// destructs before `basis::MultiConf::GetInstance()`
    /// \note moved-from object can be in any valid state,
    /// so make sure to reset `auto_registered_` in that case.
    if(auto_registered_) {
      basis::MultiConf::GetInstance().removeObserver(this);
    }
  }

  ScopedMultiConfObserver(const ScopedMultiConfObserver& other)
    : target_name_{other.target_name_}
    , cached_value_{other.cached_value_}
    , error_status_{other.error_status_}
  {
    // |observer| must not already be in the list.
    if(other.auto_registered_ && !auto_registered_) {
      basis::MultiConf::GetInstance().addObserver(this);
    }
    if(!other.auto_registered_ && auto_registered_) {
      basis::MultiConf::GetInstance().removeObserver(this);
    }
    auto_registered_ = other.auto_registered_;
  }

  ScopedMultiConfObserver(ScopedMultiConfObserver&& other)
    : target_name_{RVALUE_CAST(other.target_name_)}
    , cached_value_{RVALUE_CAST(other.cached_value_)}
    , error_status_{RVALUE_CAST(other.error_status_)}
  {
    // if moved-from type was observer -> make itself observer (if was not before)
    // if moved-from type was NOT observer -> make itself NOT observer (if was not before)
    {
      // |observer| must not already be in the list.
      if(other.auto_registered_ && !auto_registered_) {
        basis::MultiConf::GetInstance().addObserver(this);
      }
      if(!other.auto_registered_ && auto_registered_) {
        basis::MultiConf::GetInstance().removeObserver(this);
      }
    }
    // moved-from type must not be used as observer
    {
      auto_registered_ = other.auto_registered_;
      if(other.auto_registered_) {
        basis::MultiConf::GetInstance().removeObserver(&other);
      }
      other.auto_registered_ = false;
    }
  }

  ScopedMultiConfObserver& operator=(
    const ScopedMultiConfObserver& other)
  {
    target_name_ = other.target_name_;
    cached_value_ = other.cached_value_;
    error_status_ = other.error_status_;
    // |observer| must not already be in the list.
    if(other.auto_registered_ && !auto_registered_) {
      basis::MultiConf::GetInstance().addObserver(this);
    }
    if(!other.auto_registered_ && auto_registered_) {
      basis::MultiConf::GetInstance().removeObserver(this);
    }
    auto_registered_ = other.auto_registered_;
    return *this;
  }

  ScopedMultiConfObserver& operator=(
    ScopedMultiConfObserver&& other)
  {
    target_name_ = RVALUE_CAST(other.target_name_);
    cached_value_ = RVALUE_CAST(other.cached_value_);
    error_status_ = RVALUE_CAST(other.error_status_);
    // if moved-from type was observer -> make itself observer (if was not before)
    // if moved-from type was NOT observer -> make itself NOT observer (if was not before)
    {
      // |observer| must not already be in the list.
      if(other.auto_registered_ && !auto_registered_) {
        basis::MultiConf::GetInstance().addObserver(this);
      }
      if(!other.auto_registered_ && auto_registered_) {
        basis::MultiConf::GetInstance().removeObserver(this);
      }
    }
    // moved-from type must not be used as observer
    {
      auto_registered_ = other.auto_registered_;
      if(other.auto_registered_) {
        basis::MultiConf::GetInstance().removeObserver(&other);
      }
      other.auto_registered_ = false;
    }
    return *this;
  }

  void onOptionReloaded(
    const MultiConfOption& option
    , const std::string& prev_value
    , const std::string& new_value) override
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DVLOG(999)
      << "Detected change in configuration option from "
      << prev_value
      << " to "
      << new_value;

    error_status_ = basis::OkStatus();

    if (option.name == target_name_ && prev_value != new_value) {
      basis::StatusOr<T> statusor = parseValueAs<T>(new_value);
      if(statusor.ok()) {
        cached_value_ = base::rvalue_cast(statusor.ConsumeValueOrDie());
      } else {
        error_status_ = statusor.status();
      }
    }
  }

  void onCacheReloaded() override {}

  std::string id() override {
    return "ScopedMultiConfObserver";
  }

  bool is_auto_registered() const NO_EXCEPTION {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return auto_registered_;
  }

  basis::Status error_status() const NO_EXCEPTION {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return error_status_;
  }

  /// \note `GetValue()` requires `error_status().ok()`
  T GetValue(const base::Location& location = base::Location::Current()) const NO_EXCEPTION {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    CHECK(error_status_.ok())
      << location.ToString();
    return cached_value_;
  }

 private:
  template<typename Type>
  basis::StatusOr<Type> parseValueAs(
    const std::string& str) const NO_EXCEPTION;

  template<>
  basis::StatusOr<std::string> parseValueAs<std::string>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return str;
  }

  /// \note Strings "TrUe", "True", "true" and "1" will result in `true` value.
  template<>
  basis::StatusOr<bool> parseValueAs<bool>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return base::EqualsCaseInsensitiveASCII(str, "true") || str == "1";
  }

  template<>
  basis::StatusOr<int> parseValueAs<int>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    int output;
    RETURN_ERROR_IF(!base::StringToInt(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<unsigned> parseValueAs<unsigned>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    unsigned output;
    RETURN_ERROR_IF(!base::StringToUint(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<int64_t> parseValueAs<int64_t>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    int64_t output;
    RETURN_ERROR_IF(!base::StringToInt64(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<int32_t> parseValueAs<int32_t>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    int32_t output;
    RETURN_ERROR_IF(!base::StringToInt32(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<uint32_t> parseValueAs<uint32_t>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    uint32_t output;
    RETURN_ERROR_IF(!base::StringToUint32(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<uint64_t> parseValueAs<uint64_t>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    uint64_t output;
    RETURN_ERROR_IF(!base::StringToUint64(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<size_t> parseValueAs<size_t>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    size_t output;
    RETURN_ERROR_IF(!base::StringToSizeT(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<double> parseValueAs<double>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    double output;
    RETURN_ERROR_IF(!base::StringToDouble(str, &output));
    return output;
  }

  template<>
  basis::StatusOr<float> parseValueAs<float>(
    const std::string& str) const NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    double output;
    RETURN_ERROR_IF(!base::StringToDouble(str, &output));
    DCHECK(base::WithinEpsilon(output, static_cast<double>(base::saturated_cast<float>(output))))
      << "unable to store " << output << " in float type";
    /// \note Converts from double with saturation
    /// to FLOAT_MAX, FLOAT_MIN, or 0 for NaN.
    return base::saturated_cast<float>(output);
  }

 private:
  std::string target_name_;

  // if `true`, than will call in destructor
  // `basis::MultiConf::GetInstance().removeObserver(this);`
  /// \todo separate `ScopedMultiConfObserver`
  /// into `MultiConfObserver mco` and
  /// `ScopedAddObserver<MultiConf::Observer> y(&mco
  ///                      , base::BindRepeating(&basis::MultiConf::addObserver, base::Unretained(&basis::MultiConf::GetInstance()))
  ///                      , base::BindRepeating(&basis::MultiConf::removeObserver, base::Unretained(&basis::MultiConf::GetInstance()))`
  /// and rewrite `MULTICONF_String` to use
  /// template<typename T>
  /// struct MultiConfWrapper {
  ///   MultiConfObserver mco;
  ///   ScopedAddObserver<MultiConf::Observer> y;
  ///   T GetValue() { mco.GetValue(); }
  /// }
  bool auto_registered_;

  T cached_value_;

  basis::Status error_status_{basis::OkStatus()};

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);
};

} // namespace basis
