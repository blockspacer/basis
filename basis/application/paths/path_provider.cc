#include "basis/application/paths/path_provider.hpp" // IWYU pragma: associated

#include "basis/application/paths/path_id.hpp"
#include "basis/application/paths/application_paths.hpp"
#include "basis/application/paths/application_get_path.hpp"
#include "basis/application/application_configuration.hpp"

#include <base/logging.h>
#include <base/files/file.h>
#include <base/files/file_util.h>
#include <base/files/file_path.h>

namespace {
base::FilePath GetOrCreateDirectory(application::paths::AppPathId path_id)
{
  std::unique_ptr<char[]> path(new char[PLATFORM_FILE_MAX_PATH]);
  path[0] = '\0';
  if (application::paths::AppGetPath(
       path_id, path.get(), PLATFORM_FILE_MAX_PATH))
  {
    base::FilePath directory(path.get());
    if (base::PathExists(directory)
        || base::CreateDirectory(directory))
    {
      return directory;
    }
  }
  return base::FilePath();
}
}  // namespace

namespace application {

namespace paths {

bool PathProvider(int key, base::FilePath* result)
{
  DCHECK(result);

  switch (key) {
    case paths::DIR_APP_DEBUG_OUT: {
      base::FilePath directory =
          GetOrCreateDirectory(kAppPathDebugOutputDirectory);
      if (!directory.empty()) {
        *result = directory;
        return true;
      } else {
        DLOG(ERROR)
          << "Unable to get or create paths::DIR_APP_DEBUG_OUT "
          << directory.value();
        return false;
      }
    }

    case paths::DIR_APP_TEST_OUT:
      {
        base::FilePath directory =
            GetOrCreateDirectory(kAppPathTestOutputDirectory);
        if (!directory.empty()) {
          *result = directory;
          return true;
        } else {
          DLOG(ERROR)
            << "Unable to get or create paths::DIR_APP_TEST_OUT "
            << directory.value();
          return false;
        }
      }

    case paths::DIR_APP_WEB_ROOT: {
      base::FilePath directory =
          GetOrCreateDirectory(kAppPathContentDirectory);
      if (!directory.empty()) {
        *result = directory.Append("web");
        const bool dir_created
          = base::CreateDirectory(*result);
        if(!dir_created){
          DLOG(ERROR)
            << "Unable to create directory "
            << result->value();
          DCHECK(false);
        }
        return true;
      } else {
        DLOG(ERROR)
          << "Unable to get or create paths::DIR_APP_WEB_ROOT "
          << directory.value();
        return false;
      }
    }

    default:
      return false;
  }

  NOTREACHED()
    << "PathProvider failed";
  return false;
}

} // namespace paths

} // namespace application
