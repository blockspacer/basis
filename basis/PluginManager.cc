#include "basis/PluginManager.hpp" // IWYU pragma: associated

#include <Corrade/PluginManager/AbstractManager.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Directory.h>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/trace_event/trace_event.h>

#include <entt/signal/dispatcher.hpp>
#include <entt/signal/sigh.hpp>

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <utility>

#ifdef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
#error \
  "no CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT" \
  " for that platform"
#endif

namespace backend {

// extern
const char kDefaultPluginsDirName[]
  = "plugins";

// extern
const char kPluginsConfigFileName[]
  = "plugins.conf";

// extern
const char kAllPluginsConfigCategory[]
  = "plugins";

// extern
const char kIndividualPluginConfigCategory[]
  = "plugin";

using AbstractPlugin
  = ::Corrade::PluginManager::AbstractPlugin;

using AbstractManager
  = ::Corrade::PluginManager::AbstractManager;

using Configuration
  = ::Corrade::Utility::Configuration;

using ConfigurationGroup
  = ::Corrade::Utility::ConfigurationGroup;

using LoadState
  = ::Corrade::PluginManager::LoadState;

bool parsePluginsConfig(
  Configuration& conf
  , std::vector<ConfigurationGroup*>& plugin_groups
  , bool& is_plugin_filtering_enabled)
{
  if(!conf.hasGroups()) {
    VLOG(9)
      << "unable to find"
         " any configuration groups in file: "
      << conf.filename();
    return false;
  }

  if(!conf.hasGroup(backend::kAllPluginsConfigCategory)) {
    VLOG(9)
      << "unable to find"
         " configuration group: "
      << backend::kAllPluginsConfigCategory
      << " in file: "
      << conf.filename();
    return false;
  }

  is_plugin_filtering_enabled = true;

  // configurations for all plugins
  ConfigurationGroup* configurationGroup
    = conf.group(backend::kAllPluginsConfigCategory);
  DCHECK(configurationGroup);

  if(configurationGroup) {
    // configurations for individual plugins
    plugin_groups = configurationGroup->groups(
      backend::kIndividualPluginConfigCategory);
  } else {
    NOTREACHED();
  }

  return true;
}

std::vector<std::string> filterPluginsByConfig(
  std::vector<
      ::Corrade::Utility::ConfigurationGroup*
    >& plugin_groups
  , std::vector<std::string>& all_plugins)
{
  std::vector<std::string> filtered_plugins;

  for(const ConfigurationGroup* plugin_group
      : plugin_groups)
  {
    if(!plugin_group->hasValue("title"))
    {
      LOG(WARNING)
          << "invalid plugin configuration: "
          << "title not provided";
      DCHECK(false);
      continue;
    }

    const std::string plugin_conf_name
      = plugin_group->value("title");
    auto find_result
      = std::find(all_plugins.begin()
                  , all_plugins.end()
                  , plugin_conf_name);
    if(find_result == all_plugins.end())
    {
      LOG(WARNING)
          << "plugin not found: "
          << plugin_conf_name;
      NOTREACHED();
    } else {
      filtered_plugins.push_back(plugin_conf_name);
    }
  }

  return filtered_plugins;
}

} // namespace backend
