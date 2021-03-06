#include "basis/path_provider.h" // IWYU pragma: associated

#include <base/files/file_util.h>
#include <base/path_service.h>
#include <base/base_paths.h>
#include <base/files/file_path.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/notreached.h>

#include <basic/macros.h>

#include <ostream>
#include <string>

namespace basis {

namespace {

/// \note returns empty |base::FilePath()|
/// if path is NOT valid
MUST_USE_RETURN_VALUE
static ::base::FilePath GetOrCreatePath(
  const ::base::FilePath& path)
{
  if (base::PathExists(path)
      || ::base::CreateDirectory(path))
  {
    return
      path;
  }
  return
    ::base::FilePath();
}

}  // namespace

const char kAppPathDebugOutputDirectory[] = "deb_out_dir";

const char kAppPathTestOutputDirectory[] = "test_out_dir";

const char kAppPathContentDirectory[] = "content_dir";

bool PathProvider(int key, ::base::FilePath* result)
{
  DCHECK(result);

  ::base::FilePath dir_exe;
  if (!base::PathService::Get(base::DIR_EXE, &dir_exe)) {
    NOTREACHED();
  }

  switch (key) {
  case DIR_APP_DEBUG_OUT: {
    ::base::FilePath directory =
      GetOrCreatePath(dir_exe.Append(kAppPathDebugOutputDirectory));
    if (!directory.empty()) {
      *result = directory;
      return
        true;
    } else {
      DLOG(ERROR)
        << "Unable to get or create paths::DIR_APP_DEBUG_OUT "
        << directory.value();
      return
        false;
    }
  }

  case DIR_APP_TEST_OUT:
  {
    ::base::FilePath directory =
      GetOrCreatePath(dir_exe.Append(kAppPathTestOutputDirectory));
    if (!directory.empty()) {
      *result = directory;
      return
        true;
    } else {
      DLOG(ERROR)
        << "Unable to get or create paths::DIR_APP_TEST_OUT "
        << directory.value();
      return
        false;
    }
  }

  case DIR_APP_WEB_ROOT: {
    ::base::FilePath directory =
      GetOrCreatePath(dir_exe.Append(kAppPathContentDirectory));
    if (!directory.empty()) {
      *result = directory.Append("web");
      const bool dir_created
        = ::base::CreateDirectory(*result);
      if(!dir_created) {
        DLOG(ERROR)
            << "Unable to create directory "
            << result->value();
        DCHECK(false);
      }
      return
        true;
    } else {
      DLOG(ERROR)
        << "Unable to get or create paths::DIR_APP_WEB_ROOT "
        << directory.value();
      return
        false;
    }
  }

  default:
    return
      false;
  }

  NOTREACHED()
    << "PathProvider failed";
  return
    false;
}

void AddPathProvider()
{
  ::base::PathService::RegisterProvider(
    &basis::PathProvider
    , ::basis::PATH_APP_START
    , ::basis::PATH_APP_END);

  ::base::FilePath log_directory;
  ::base::PathService::Get(
    ::basis::DIR_APP_DEBUG_OUT, &log_directory);
  VLOG(9)
    << "log_directory: "
    << log_directory.value();

  ::base::FilePath test_root_directory;
  ::base::PathService::Get(
    ::basis::DIR_APP_TEST_OUT, &test_root_directory);
  VLOG(9)
    << "test_root_directory: "
    << test_root_directory.value();

  ::base::FilePath web_root_directory;
  ::base::PathService::Get(
    ::basis::DIR_APP_WEB_ROOT, &web_root_directory);
  VLOG(9)
    << "web_root_directory: "
    << web_root_directory.value();
}

} // namespace basis
