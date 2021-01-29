#include "basis/multiconfig/multiconfig.hpp"  // IWYU pragma: associated

#include <base/environment.h>
#include <base/location.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <base/stl_util.h>
#include <base/bind.h>
#include <base/values.h>
#include <base/files/file_util.h>
#include <base/json/json_string_value_serializer.h>
#include <base/json/json_reader.h>
#include <base/threading/scoped_blocking_call.h>
#include <base/command_line.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>

namespace basis {

namespace {

static const size_t kMaxDebugLogItemSize = 9999;

basis::StatusOr<base::Value> parseJSONData(
  const std::string& json_data)
{
  // This could be really slow.
  base::ScopedBlockingCall scoped_blocking_call(
    FROM_HERE, base::BlockingType::MAY_BLOCK);

  base::JSONReader::ValueWithError value_with_error =
    base::JSONReader::ReadAndReturnValueWithError(
      json_data
      , base::JSON_PARSE_RFC);

  if (!value_with_error.value) {
    RETURN_ERROR()
      << "Failed to parse JSON: "
      << "JSON error "
      << base::StringPrintf(
           "%s (%d:%d)"
           , value_with_error.error_message.c_str()
           , value_with_error.error_line
           ,  value_with_error.error_column)
      << " JSON data starts with: "
      << json_data.substr(0, kMaxDebugLogItemSize)
      << " ...";
  }

  base::Value& root
    = value_with_error.value.value();

  if (!root.is_dict()) {
    RETURN_ERROR()
      << "Failed to parse JSON:"
      << " Root item must be a dictionary."
      << " But it is: "
      << base::Value::GetTypeName(root.type())
      << " and it has type index: "
      << static_cast<size_t>(root.type())
      << " JSON data starts with: "
      << json_data.substr(0, kMaxDebugLogItemSize)
      << " ...";
  }

  return std::move(value_with_error.value.value());
}

} // namespace

std::string formatConfigNameAndGroup(
  const std::string&name, const std::string group)
{
  return (name + "_" + group);
  //return group.empty()
  //  ? name
  //  : (name + "_" + group);
}

EnvMultiConf::EnvMultiConf()
  : env_{base::Environment::Create()}
{}

EnvMultiConf& EnvMultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<EnvMultiConf> instance;
  return *instance;
}

basis::StatusOr<std::string> EnvMultiConf::tryLoadString(
  const std::string& name, const std::string& configuration_group)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string key = formatConfigNameAndGroup(name, configuration_group);

  DCHECK(!key.empty());

  std::string result;

  if(env_->GetVar(key, &result))
  {
    return result;
  }

  if(env_->GetVar(base::ToUpperASCII(key), &result))
  {
    return result;
  }

  if(env_->GetVar(base::ToLowerASCII(key), &result))
  {
    return result;
  }

  RETURN_ERROR()
    << "unable to find key in environment variables: "
    << key
    << " in loader "
    << kId;
}

CmdMultiConf::CmdMultiConf()
  : command_line_{base::CommandLine::ForCurrentProcess()}
{}

CmdMultiConf& CmdMultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<CmdMultiConf> instance;
  return *instance;
}

basis::StatusOr<std::string> CmdMultiConf::tryLoadString(
  const std::string& name, const std::string& configuration_group)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string key = formatConfigNameAndGroup(name, configuration_group);

  DCHECK(!key.empty());

  if(command_line_->HasSwitch(key))
  {
    return command_line_->GetSwitchValueASCII(key);
  }

  if(std::string upper = base::ToUpperASCII(key); command_line_->HasSwitch(upper))
  {
    return command_line_->GetSwitchValueASCII(upper);
  }

  if(std::string lower = base::ToLowerASCII(key); command_line_->HasSwitch(lower))
  {
    return command_line_->GetSwitchValueASCII(lower);
  }

  RETURN_ERROR()
    << "unable to find key in environment variables: "
    << key
    << " in loader "
    << kId;
}

JsonMultiConf::JsonMultiConf()
{}

JsonMultiConf& JsonMultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<JsonMultiConf> instance;
  return *instance;
}

basis::Status JsonMultiConf::clearAndParseFromFilePath(
  const base::FilePath& file_path)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string json_data;

  // Failure to read the file is ignored
  // as |json_data| will be the empty string
  // and the remainder of this object should still be
  // initialized as best as possible.
  if (!base::PathExists(file_path))
  {
    RETURN_ERROR()
      << "File does not exist: "
      << file_path.value()
      << " in loader "
      << kId;

    /// \note init from empty |json_data| string
  } else if (
      base::PathExists(file_path)
      && !base::ReadFileToString(file_path, &json_data))
  {
    RETURN_ERROR()
      << "Failed to read JSON from file: "
      << file_path.value()
      << " in loader "
      << kId;

    /// \note clearAndParseFromString will be called
    /// with empty |json_data| string
  }

  /// \note resets cache even in case of loading error
  cached_dictionary_.reset();

  RETURN_WITH_MESSAGE_IF_NOT_OK(clearAndParseFromString(json_data))
    << "Failed to parse JSON from file:"
    << file_path;

  RETURN_OK();
}

std::string JsonMultiConf::serializeCachedConfig() const
{
  if(!cached_dictionary_.has_value()) return "";
  // Serialize back the |cached_dictionary_|.
  std::string serialized_json;
  JSONStringValueSerializer str_serializer(&serialized_json);
  str_serializer.set_pretty_print(true);
  const bool serializeOk
    = str_serializer.Serialize(cached_dictionary_.value());
  DCHECK(serializeOk);
  return serialized_json;
}

basis::Status JsonMultiConf::clearAndParseFromString(
  const std::string& json_data)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  if (json_data.empty()) {
    RETURN_ERROR()
      << "empty JSON"
      << " in loader "
      << kId;
  }

  /// \note resets cache even in case of loading error
  cached_dictionary_.reset();

  // will hold the nullptr in case of an error.
  CONSUME_OR_RETURN_WITH_MESSAGE(cached_dictionary_
    , parseJSONData(json_data), std::string{"failed_to_parse_JSON_string"});

  RETURN_OK();
}

basis::StatusOr<std::string> JsonMultiConf::tryLoadString(
  const std::string& name, const std::string& configuration_group)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string key = formatConfigNameAndGroup(name, configuration_group);

  DCHECK(!key.empty());

  if(!cached_dictionary_.has_value())
  {
    RETURN_ERROR()
      << "json configuration not loaded"
      << key
      << " in loader "
      << kId;
  }

  DCHECK(cached_dictionary_.value().is_dict());
  if(!cached_dictionary_.value().is_dict())
  {
    RETURN_ERROR()
      << "invalid json configuration for key: "
      << key
      << " in loader "
      << kId;
  }

  const std::string* foundValue
    = cached_dictionary_.value().FindStringKey(key);
  if(foundValue == nullptr)
  {
    RETURN_ERROR()
      << "unable to find key in json configuration: "
      << key
      << " in loader "
      << kId;
  }

  return *foundValue;
}

MultiConfOption::MultiConfOption(
  const std::string& key
  , const base::Optional<std::string>& default_value
  , const std::initializer_list<MultiConfLoader>& loaders
  , const std::string& configuration_group)
  : name(key)
  , default_str(default_value)
  , loaders(loaders)
  , configuration_group(configuration_group)
{}

MultiConf::Observer::Observer() = default;

MultiConf::Observer::~Observer() = default;

MultiConf::MultiConf()
  : observers_(new ::base::ObserverListThreadSafe<MultiConf::Observer>())
{}

MultiConf::~MultiConf() {
  AssertObserversEmpty();
}

void MultiConf::notifyOptionReloaded(
  const MultiConfOption& option
  , const std::string& prev_value
  , const std::string& new_value)
{
  DCHECK(observers_);

  /// \note thread-safe, so skip `debug_thread_collision_warner_`
  observers_->Notify(FROM_HERE
    , &MultiConf::Observer::onOptionReloaded
    , option
    , prev_value
    , new_value);
}

void MultiConf::notifyCacheReloaded()
{
  DCHECK(observers_);

  /// \note thread-safe, so skip `debug_thread_collision_warner_`
  observers_->Notify(FROM_HERE, &MultiConf::Observer::onCacheReloaded);
}

void MultiConf::addObserver(
  MultiConf::Observer* observer)
{
  DCHECK(observer);
  DCHECK(observers_);

  DVLOG(999)
    << "Added observer "
    << observer->id();

  /// \note thread-safe, so skip `debug_thread_collision_warner_`
  observers_->AddObserver(observer);
}

void MultiConf::removeObserver(
  MultiConf::Observer* observer)
{
  DCHECK(observer);
  DCHECK(observers_);

  DVLOG(999)
    << "Removed observer "
    << observer->id();

  /// \note thread-safe, so skip `debug_thread_collision_warner_`
  observers_->RemoveObserver(observer);
}

MultiConf& MultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<MultiConf> instance;
  return *instance;
}

basis::Status MultiConf::addOption(const MultiConfOption& option)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  CHECK_GT(option.loaders.size(), 0)
    << "No configuration loaders provided for option:"
    << formatConfigNameAndGroup(option.name, option.configuration_group);

  auto [iter, is_newly_inserted] = known_config_options_.insert(option);
  RETURN_ERROR_IF(!is_newly_inserted)
    << "Failed to add configuration option twice: "
    << formatConfigNameAndGroup(option.name, option.configuration_group);
  DCHECK(iter != known_config_options_.end());
  RETURN_OK();
}

bool MultiConf::hasOptionWithName(const std::string& name
  , const std::string& configuration_group)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  // creates dummy option to find real option
  return known_config_options_.find(
    MultiConfOption{name, base::nullopt, {}, configuration_group}) != known_config_options_.end();
}

basis::Status MultiConf::reloadOptionWithName(
  const std::string& name
  , const std::string& configuration_group
  , bool notify_cache_reload_on_success
  , bool clear_cache_on_error)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  // creates dummy option to find real option
  auto it = known_config_options_.find(MultiConfOption{name, base::nullopt, {}, configuration_group});
  RETURN_ERROR_IF(it == known_config_options_.end())
    << "Failed to find configuration option: "
    << formatConfigNameAndGroup(name, configuration_group);

  RETURN_IF_NOT_OK(reloadOption(*it, notify_cache_reload_on_success, clear_cache_on_error));

  RETURN_OK();
}

basis::Status MultiConf::clearAndReload(bool clear_cache_on_error)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  current_config_cache_.clear();

  if(known_config_options_.empty()) {
    LOG_EVERY_N_MS(WARNING, 100)
      << "No configuration options provided.";
  }

  /// \note loading order is not guaranteed to be same as user may want
  for(const MultiConfOption& option: known_config_options_) {
    /// \note Does not return error if configuration option has default value.
    RETURN_IF_NOT_OK(reloadOption(option
      // no need to call `notifyCacheReloaded` on success
      // because we will call it below anyway
      , false
      , clear_cache_on_error));
    DVLOG(999)
      << "Reloaded configuration value: "
      << formatConfigNameAndGroup(option.name, option.configuration_group);
  }

  notifyCacheReloaded();

  RETURN_OK();
}

basis::Status MultiConf::reloadOption(
  const MultiConfOption& option
  , bool notify_cache_reload_on_success
  , bool clear_cache_on_error)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string prev_value
    = base::FindWithDefault(
        current_config_cache_
        , formatConfigNameAndGroup(option.name, option.configuration_group)
        , option.default_str.has_value() ? option.default_str.value() : "");

  DCHECK_GT(option.loaders.size(), 0)
    << "No configuration loaders provided for option:"
    << formatConfigNameAndGroup(option.name, option.configuration_group);

  basis::StatusOr<std::string> statusor = loadAsStringWithoutDefaults(option);
  if(statusor.ok()) {
    current_config_cache_[formatConfigNameAndGroup(option.name, option.configuration_group)]
      = statusor.ConsumeValueOrDie();
  } else {
    if(!option.default_str.has_value()) {
      if(notify_cache_reload_on_success) {
        notifyCacheReloaded();
      }
      if(clear_cache_on_error) {
        /// \note Resets whole cache in case of any error.
        /// We assume that each hard-coded configuration option is important
        /// and required to proceed.
        current_config_cache_.clear();
        // cache corrupted, need to notify about change anyway
        notifyCacheReloaded();
      }
      RETURN_ERROR()
        << "Failed to load configuration value: "
        << formatConfigNameAndGroup(option.name, option.configuration_group);
    }
    DVLOG(999)
      << "Configuration value: "
      << formatConfigNameAndGroup(option.name, option.configuration_group)
      << " uses default value: "
      << option.default_str.value();
    current_config_cache_[formatConfigNameAndGroup(option.name, option.configuration_group)]
      = option.default_str.value();
  }

  notifyOptionReloaded(option
    , prev_value
    , current_config_cache_[formatConfigNameAndGroup(option.name, option.configuration_group)]);

  if(notify_cache_reload_on_success) {
    notifyCacheReloaded();
  }

  RETURN_OK();
}

basis::StatusOr<std::string> MultiConf::getAsStringFromCache(
  const std::string& name
  , const std::string& configuration_group)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string* result
    = base::FindOrNull(current_config_cache_
      , formatConfigNameAndGroup(name, configuration_group));
  if (!result) {
    RETURN_ERROR()
      << "Unable to find cached configuration value "
      << formatConfigNameAndGroup(name, configuration_group)
      << ". Maybe you forgot to reload configuration"
      << " or configuration is broken?";
  }

  return *result;
}

basis::StatusOr<std::string> MultiConf::loadAsStringWithoutDefaults(
  const MultiConfOption& option)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  DCHECK_GT(option.loaders.size(), 0)
    << "No configuration loaders provided for option:"
    << formatConfigNameAndGroup(option.name, option.configuration_group);

  /// \note loading order is not guaranteed to be same as user may want
  for(const MultiConfLoader& loader: option.loaders) {
    basis::StatusOr<std::string> result
      = loader.func.Run(option.name, option.configuration_group);
    if (result.ok()) {
      DVLOG(999)
        << "Configuration value: "
        << formatConfigNameAndGroup(option.name, option.configuration_group)
        << " uses loader: "
        << loader.name;
      return result;
    }
  }

  RETURN_ERROR()
    << "Failed to find configuration value: "
    << formatConfigNameAndGroup(option.name, option.configuration_group)
    << " Count of used loaders: "
    << option.loaders.size();
}

} // namespace basis
