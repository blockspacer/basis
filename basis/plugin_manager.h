#pragma once

#include <Corrade/Containers/Pointer.h>
#include <Corrade/PluginManager/AbstractManager.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Directory.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include <base/macros.h>

#include <basic/macros.h>
#include "basic/rvalue_cast.h"

#include <entt/signal/dispatcher.hpp>
#include <entt/signal/sigh.hpp>

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <utility>
#include <cstddef>
#include <string>
#include <vector>

namespace entt {
class dispatcher;
} // namespace entt

namespace backend {

struct PluginManagerEvents {
  struct Startup {
    ::base::FilePath pathToDirWithPlugins;
    ::base::FilePath pathToPluginsConfFile;
    std::vector<::base::FilePath> pathsToExtraPluginFiles;
  };
  struct Shutdown {
    // event parameters
  };
};

extern
const char kDefaultPluginsDirName[];

extern
const char kPluginsConfigFileName[];

extern
const char kAllPluginsConfigCategory[];

extern
const char kIndividualPluginConfigCategory[];

MUST_USE_RETURN_VALUE
bool parsePluginsConfig(
  ::Corrade::Utility::Configuration& conf
  , std::vector<
      ::Corrade::Utility::ConfigurationGroup*
    >& plugin_groups
  , bool& is_plugin_filtering_enabled);

std::vector<std::string> filterPluginsByConfig(
  std::vector<
      ::Corrade::Utility::ConfigurationGroup*
    >& plugin_groups
  , std::vector<std::string>& all_plugins);

template<
  typename PluginType
>
class PluginManager {
public:
  using PluginPtr
    = Corrade::Containers::Pointer<
        PluginType
      >;

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

public:
  PluginManager()
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  ~PluginManager() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    /// \note destoy |entt::dispatcher| before plugin manager
  }

  // |dispatcher| to handle |PluginManager|
  // events like |Startup| or |Shutdown|
  void connect_to_dispatcher(
    entt::dispatcher& events_dispatcher)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    TRACE_EVENT0("toplevel", "PluginManager::connect_to_dispatcher()")

    events_dispatcher.sink<PluginManagerEvents::Startup>()
      .template connect<&PluginManager::startup>(this);
    events_dispatcher.sink<PluginManagerEvents::Shutdown>()
      .template connect<&PluginManager::shutdown>(this);
  }

  // |dispatcher| to handle per-plugin events
  void connect_plugins_to_dispatcher(
    entt::dispatcher& events_dispatcher)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    TRACE_EVENT0("toplevel", "PluginManager::connect_to_dispatcher()")

    for(PluginPtr& loaded_plugin : loaded_plugins_) {
      DCHECK(loaded_plugin);
      loaded_plugin->connect_to_dispatcher(events_dispatcher);
    }
  }

  /// \todo refactor long method
  void startup(const PluginManagerEvents::Startup& event)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    TRACE_EVENT0("toplevel", "PluginManager::startup()")

    VLOG(9) << "(PluginManager) startup";

    using namespace ::Corrade::Utility::Directory;

    const std::string executable_path
      = path(
          executableLocation());
    CHECK(!executable_path.empty())
      << "invalid executable path";

    const ::base::FilePath pathToDirWithPlugins
      = event.pathToDirWithPlugins.empty()
        // default value
          ? ::base::FilePath{executable_path}
          : event.pathToDirWithPlugins;
    CHECK(!pathToDirWithPlugins.empty())
      << "invalid path to directory with plugins";

    const std::string pluginsConfFile
      = join(
          std::initializer_list<std::string>{
            pathToDirWithPlugins.value()
            , kPluginsConfigFileName}
          );

    const ::base::FilePath pathToPluginsConfFile
      = event.pathToPluginsConfFile.empty()
        // default value
        ? ::base::FilePath{pluginsConfFile}
        : event.pathToPluginsConfFile;
    CHECK(!pathToPluginsConfFile.empty())
      << "invalid path to plugins configuration file";

    VLOG(9)
      << "using plugins configuration file: "
      << pathToPluginsConfFile;

    Configuration conf{
      pathToPluginsConfFile.value()};

    std::vector<ConfigurationGroup*> plugin_groups;

    // filter plugins based on config
    bool is_plugin_filtering_enabled = false;

    // parse plugins configuration file
    {
      const bool parseOk = parsePluginsConfig(conf
        , plugin_groups
        , is_plugin_filtering_enabled
      );

      if(!parseOk) {
        LOG(WARNING)
          << "unable to parse plugins configuration file: "
          << pathToPluginsConfFile;
      }
    }

    DCHECK(!manager_);
    manager_
      = std::make_unique<
      ::Corrade::PluginManager::Manager<PluginType>
      >();

    /**
     * Set the plugin manager directory to load plugins
     *
     * Keeps loaded plugins untouched, removes unloaded plugins which are
     * not existing anymore and adds newly found plugins. The directory is
     * expected to be in UTF-8.
     * @partialsupport Not available on platforms without
     *      @ref CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT "dynamic plugin support".
     */
    manager_->setPluginDirectory(
      event.pathToDirWithPlugins.value());

    VLOG(9)
      << "Using plugin directory: "
      << manager_->pluginDirectory();

    std::vector<std::string> all_plugins
      = manager_->pluginList();

    std::vector<std::string> filtered_plugins;

    // parse list of plugin sections from plugins configuration file
    if(is_plugin_filtering_enabled)
    {
      filtered_plugins
        = filterPluginsByConfig(plugin_groups
            , all_plugins
          );
    }

    // append path to plugins that
    // must be loaded independently of configuration file
    for(const ::base::FilePath& pluginPath
        : event.pathsToExtraPluginFiles)
    {
      VLOG(9)
          << "added plugin: "
          << pluginPath;
      if(pluginPath.empty() || !base::PathExists(pluginPath)) {
        LOG(ERROR)
          << "invalid path to plugin file: "
          << pluginPath;
        CHECK(false)
          << "path does not exist: "
          << pluginPath;
      }
      filtered_plugins.push_back(
        /*
        * @note If passing a file path, the implementation expects forward
        *      slashes as directory separators.
        * Use @ref Utility::Directory::fromNativeSeparators()
        *      to convert from platform-specific format.
        */
        fromNativeSeparators(
          pluginPath.value()));
    }

    DCHECK(loaded_plugins_.empty())
      << "Plugin manager must load plugins once.";

    for(std::vector<std::string>::const_iterator it =
          filtered_plugins.begin()
        ; it != filtered_plugins.end(); ++it)
    {
      const std::string& pluginNameOrPath = *it;

      // The implementation expects forward slashes as directory separators.
      DCHECK(fromNativeSeparators(
               pluginNameOrPath) == pluginNameOrPath);

      VLOG(9)
          << "plugin enabled: "
          << pluginNameOrPath;

      /**
       * @brief Load a plugin
       *
       * Returns @ref LoadState::Loaded if the plugin is already loaded or if
       * loading succeeded. For static plugins returns always
       * @ref LoadState::Static. On failure returns @ref LoadState::NotFound,
       * @ref LoadState::WrongPluginVersion,
       * @ref LoadState::WrongInterfaceVersion,
       * @ref LoadState::UnresolvedDependency or @ref LoadState::LoadFailed.
       *
       * If the plugin has any dependencies, they are recursively processed
       * before loading given plugin.
       *
       * If @p plugin is a plugin file path (i.e., ending with a
       * platform-specific extension such as `.so` or `.dll`), it's loaded
       * from given location and, if the loading succeeds, its basename
       * (without extension) is exposed as an available plugin name. It's
       * expected that a plugin with the same name is not already loaded. The
       * plugin will reside in the plugin list as long as it's loaded or,
       * after calling @ref unload() on it, until next call to
       * @ref setPluginDirectory() or @ref reloadPluginDirectory().
       *
       * @note If passing a file path, the implementation expects forward
       *      slashes as directory separators. Use @ref Utility::Directory::fromNativeSeparators()
       *      to convert from platform-specific format.
       *
       * @see @ref unload(), @ref loadState(), @ref Manager::instantiate(),
       *      @ref Manager::loadAndInstantiate()
       * @partialsupport On platforms without
       *      @ref CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT "dynamic plugin support"
       *      returns always either @ref LoadState::Static or
       *      @ref LoadState::NotFound.
      **/
      DCHECK(manager_);
      const bool is_loaded
        = static_cast<bool>(
            manager_->load(pluginNameOrPath)
            & (LoadState::Loaded
               | LoadState::
                 Static)
            );
      if(!is_loaded) {
        LOG(ERROR)
          << "The requested plugin "
          << pluginNameOrPath
          << " cannot be loaded.";
        DCHECK(false);
        continue;
      }

      /**
      @brief Extract filename (without path) from filename

      If the filename doesn't contain any slash, returns whole string, otherwise
      returns everything after last slash.
      @attention The implementation expects forward slashes as directory separators.
          Use @ref fromNativeSeparators() to convert from a platform-specific format.
      @see @ref path(), @ref splitExtension()
      */
      const std::string pluginNameOrFilenameWithExt
        = filename(pluginNameOrPath);

      DCHECK(base::FilePath{pluginNameOrPath}
               .BaseName().value()
             == pluginNameOrFilenameWithExt);

      /**
      @brief Split basename and extension
      @m_since{2019,10}

      Returns a pair `(root, ext)` where @cpp root + ext == path @ce, and ext is
      empty or begins with a period and contains at most one period. Leading periods
      on the filename are ignored, @cpp splitExtension("/home/.bashrc") @ce returns
      @cpp ("/home/.bashrc", "") @ce. Behavior equivalent to Python's
      @cb{.py} os.path.splitext() @ce.
      @attention The implementation expects forward slashes as directory separators.
          Use @ref fromNativeSeparators() to convert from a platform-specific format.
      @see @ref path(), @ref filename(), @ref String::partition()
      */
      const std::string pluginName
        = splitExtension(
            pluginNameOrFilenameWithExt).first;

      DCHECK(base::FilePath{pluginNameOrFilenameWithExt}
               .BaseName().RemoveExtension().value()
             == pluginName);

      DCHECK(manager_ && static_cast<bool>(
               manager_->loadState(pluginName)
               & (LoadState::
                    Loaded
                  | LoadState::
                    Static)
               ));

      /// Returns new instance of given plugin.
      /// \note The plugin must be already
      /// successfully loaded by this manager.
      /// \note The returned value is never |nullptr|
      DCHECK(manager_);
      PluginPtr plugin
      /// \note must be plugin name, not path to file
        = manager_->instantiate(pluginName);
      if(!plugin) {
        LOG(ERROR)
          << "The requested plugin "
          << pluginNameOrPath
          << " cannot be instantiated.";
        DCHECK(false);
        continue;
      }

      VLOG(9)
        << "=== loading plugin ==";
      VLOG(9)
          << "plugin title:       "
          << plugin->title();
      VLOG(9)
          << "plugin description: "
          << plugin->description().substr(0, 100)
          << "...";

      plugin->load();

      loaded_plugins_.push_back(RVALUE_CAST(plugin));
      VLOG(9)
        << "=== plugin loaded ==";
    }

    DCHECK(!is_initialized_)
      << "Plugin manager must be initialized once."
      << "You can unload or reload plugins at runtime.";
    is_initialized_ = true;
  }

  void shutdown(const PluginManagerEvents::Shutdown&)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    TRACE_EVENT0("toplevel", "PluginManager::shutdown()");

    VLOG(9) << "(PluginManager) shutdown";

    /// \note destructor of ::Corrade::PluginManager::Manager
    /// also unloads all plugins
    for(PluginPtr& loaded_plugin : loaded_plugins_) {
      DCHECK(loaded_plugin);
      loaded_plugin->unload();
      //loaded_plugin.reset(nullptr);
    }
  }

  MUST_USE_RETURN_VALUE
  size_t countLoadedPlugins()
  const noexcept
  {
    return
      loaded_plugins_.size();
  }

private:
  bool is_initialized_ = false;

  std::unique_ptr<
    ::Corrade::PluginManager::Manager<PluginType>
    > manager_;

  std::vector<PluginPtr> loaded_plugins_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PluginManager);
};

} // namespace backend
