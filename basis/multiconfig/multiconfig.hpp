#pragma once

#include "basis/status/status_macros.hpp"
#include "basis/core/observable.hpp"

#include <base/logging.h>
#include <base/values.h>
#include <base/callback_forward.h>
#include <base/bind.h>
#include <base/base_export.h>
#include <base/macros.h>
#include <base/no_destructor.h>
#include <base/optional.h>
#include <base/rvalue_cast.h>
#include <base/threading/thread_collision_warner.h>

#include <string>
#include <ostream>

#define ENV_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::EnvMultiConf::kId \
      , base::BindRepeating(&::basis::EnvMultiConf::tryLoadString) \
    }

/// \todo
//    static basis::Observable<int> value_;
//  basis::Observer<int> Observe() { return value_.Observe(); }
//    value_.SetValue(value);

#define DEFAULT_MULTICONF_LOADERS \
  { \
    ENV_MULTICONF_LOADER \
  }

// USAGE
//
// MULTICONF_String(my_conf_key, "abcd", DEFAULT_MULTICONF_LOADERS);
//
#define MULTICONF_String(KEY_NAME, DEFAULT_STR, ...) \
  basis::MultiConf::GetInstance().addEntry(basis::MultiConfEntry{ \
    KEY_NAME \
    , DEFAULT_STR \
    , __VA_ARGS__ \
  });

namespace basis {

// Configuration loader that uses environment vars
struct EnvMultiConf {
  // Loads configuraton value from environment vars in order:
  // * key
  // * uppercase(key)
  // * lowercase(key)
  static basis::StatusOr<std::string> tryLoadString(const std::string& key);

  // id for debug purposes
  static constexpr char kId[] = "EnvMultiConf";
};

// Wraps functions from any configuration loader
struct MultiConfLoader {
  using LoaderFunc
    = base::RepeatingCallback<
        basis::StatusOr<std::string>(const std::string& key)
      >;

  std::string name;
  LoaderFunc func;
};

struct MultiConfEntry
{
 public:
  MultiConfEntry(
    const std::string& key
    , const base::Optional<std::string>& default_value
    , const std::initializer_list<MultiConfLoader>& loaders);

  friend bool operator<(const MultiConfEntry& a, const MultiConfEntry& b) {
    return a.name < b.name;
  }

  friend std::ostream& operator<<(std::ostream& out, const MultiConfEntry& i) {
    return out << i.name;
  }

  std::string name;
  base::Optional<std::string> default_str = base::nullopt;
  std::vector<MultiConfLoader> loaders;
};

class MultiConf {
 public:
  // Thread safe GetInstance.
  static MultiConf& GetInstance();

  basis::Status init() {
    DCHECK_EQ(current_config_cache_.size(), 0);
    RETURN_WITH_MESSAGE_IF_NOT_OK(clearAndReload())
      << "Failed to initialize configuration";
    RETURN_OK();
  }

  basis::Status addEntry(const MultiConfEntry& entry);

  // Updates config based on current content of config files, env. vars, etc.
  // Populates `current_config_cache_`
  basis::Status clearAndReload();

  // Finds key with provided name in `current_config_cache_`
  basis::StatusOr<std::string> getAsStringFromCache(const std::string& name);

 private:
  // Calls each loader in `MultiConfEntry` until it returns value.
  basis::StatusOr<std::string> tryLoadString(
    const MultiConfEntry& entry);

 private:
  MultiConf();

  friend class base::NoDestructor<MultiConf>;

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  std::set<MultiConfEntry> known_config_entries_;

  std::map<std::string, std::string> current_config_cache_;

  DISALLOW_COPY_AND_ASSIGN(MultiConf);
};

} // namespace basis
