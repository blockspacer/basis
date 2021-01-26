#include "basis/multiconfig/multiconfig.hpp"  // IWYU pragma: associated

#include <base/environment.h>
#include <base/location.h>
#include <base/strings/string_util.h>
#include <base/stl_util.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>

namespace basis {

// static
basis::StatusOr<std::string> EnvMultiConf::tryLoadString(const std::string& key)
{
  DCHECK(!key.empty());

  std::string result;

  std::unique_ptr<base::Environment> env_var_getter(
    base::Environment::Create());

  if(env_var_getter->GetVar(key, &result))
  {
    return result;
  }

  if(env_var_getter->GetVar(base::ToUpperASCII(key), &result))
  {
    return result;
  }

  if(env_var_getter->GetVar(base::ToLowerASCII(key), &result))
  {
    return result;
  }

  RETURN_ERROR()
    << FROM_HERE.ToString()
    << " unable to find env. key: "
    << key;
}

MultiConfEntry::MultiConfEntry(
  const std::string& key
  , const base::Optional<std::string>& default_value
  , const std::initializer_list<MultiConfLoader>& loaders)
  : name(key)
  , default_str(default_value)
  , loaders(loaders)
{}

MultiConf::MultiConf()
{}

MultiConf& MultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<MultiConf> instance;
  return *instance;
}

basis::Status MultiConf::addEntry(const MultiConfEntry& entry)
{
  auto [iter, is_newly_inserted] = known_config_entries_.insert(entry);
  RETURN_ERROR_IF(!is_newly_inserted);
  DCHECK(iter != known_config_entries_.end());
  RETURN_OK();
}

basis::Status MultiConf::clearAndReload()
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  current_config_cache_.clear();

  if(known_config_entries_.empty()) {
    LOG_EVERY_N_MS(WARNING, 100)
      << "No configuration entries provided.";
  }

  for(const MultiConfEntry& entry: known_config_entries_) {
    basis::StatusOr<std::string> statusor = tryLoadString(entry);
    if(statusor.ok()) {
      current_config_cache_[entry.name] = statusor.ConsumeValueOrDie();
    } else {
      if(!entry.default_str.has_value()) {
        /// \note resets whole cache in case of any error
        current_config_cache_.clear();
        RETURN_ERROR()
          << "Failed to load configuration value: "
          << entry.name;
      }
      current_config_cache_[entry.name] = entry.default_str.value();
    }
  }

  RETURN_OK();
}

basis::StatusOr<std::string> MultiConf::getAsStringFromCache(
  const std::string& name)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  std::string* result = base::FindOrNull(current_config_cache_, name);
  if (!result) {
    RETURN_ERROR()
      << "Unable to find cached configuration value "
      << name
      << ". Maybe you forgot to reload configuration"
      << " or configuration is broken?";
  }

  return *result;
}

basis::StatusOr<std::string> MultiConf::tryLoadString(
  const MultiConfEntry& entry)
{
  DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

  for(const MultiConfLoader& loader: entry.loaders) {
    basis::StatusOr<std::string> result = loader.func.Run(entry.name);
    if (result.ok()) {
      return result;
    }
  }

  RETURN_ERROR();
}

} // namespace basis
