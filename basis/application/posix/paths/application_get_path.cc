#include "basis/application/paths/application_get_path.h" // IWYU pragma: associated

#include "basis/application/application_configuration.h"

#include <cstring>

#include <base/logging.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/strings/stringprintf.h>
#include <base/path_service.h>
#include <base/strings/safe_sprintf.h>
#include <base/notreached.h>

namespace {

static const int kMaxPathSize = PLATFORM_FILE_MAX_PATH;

// Gets the path to the cache directory, using the user's home directory.
bool GetCacheDirectory(char* out_path, int path_size) {
  const ::base::FilePath home_dir = ::base::GetHomeDir();

  if (home_dir.empty())
  {
    DCHECK(false);
    return false;
  }

  /// \todo use ::base::strlcpy
  const int result = ::base::strings::SafeSNPrintf(
    out_path, path_size, "%s/.cache", home_dir.value().c_str());
  if (result < 1 || result > kMaxPathSize) {
    out_path[0] = '\0';
    DCHECK(false);
    return false;
  }

  const bool dir_created
    = ::base::CreateDirectory(base::FilePath{out_path});
  if(!dir_created){
    DLOG(ERROR)
      << "Unable to create directory "
      << out_path;
    DCHECK(false);
  }

  return dir_created;
}

// Places up to |path_size| - 1 characters of the path to the current
// executable in |out_path|, ensuring it is NULL-terminated. Returns success
// status. The result being greater than |path_size| - 1 characters is a
// failure. |out_path| may be written to in unsuccessful cases.
bool GetExecutablePath(char* out_path, int path_size) {
  if (path_size < 1) {
    DCHECK(false);
    return false;
  }

  char path[kMaxPathSize + 1];
  ssize_t bytes_read = readlink("/proc/self/exe", path, kMaxPathSize);
  if (bytes_read < 1) {
    DCHECK(false);
    return false;
  }

  path[bytes_read] = '\0';
  if (bytes_read > path_size) {
    DCHECK(false);
    return false;
  }

  /// \todo use ::base::strlcpy
  const int result = ::base::strings::SafeSNPrintf(
    out_path, path_size, "%s", path);
  if (result < 1 || result > kMaxPathSize) {
    out_path[0] = '\0';
    DCHECK(false);
    return false;
  }

  return true;
}

// Places up to |path_size| - 1 characters of the path to the directory
// containing the current executable in |out_path|, ensuring it is
// NULL-terminated. Returns success status. The result being greater than
// |path_size| - 1 characters is a failure. |out_path| may be written to in
// unsuccessful cases.
bool GetExecutableDirectory(char* out_path, int path_size) {
  if (!GetExecutablePath(out_path, path_size)) {
    DCHECK(false);
    return false;
  }

  /// \todo memory copy
  const ::base::FilePath fp = ::base::FilePath(out_path);

  /// \todo use ::base::strlcpy
  const int result
    = ::base::strings::SafeSNPrintf(
        out_path
        , path_size
        , "%s"
        , fp.DirName().value().c_str());
  if (result < 1 || result > kMaxPathSize) {
    out_path[0] = '\0';
    DCHECK(false);
    return false;
  }

  return true;
}

// Gets only the name portion of the current executable.
bool GetExecutableName(char* out_path, int path_size) {
  char path[kMaxPathSize] = {0};
  if (!GetExecutablePath(path, kMaxPathSize)) {
    DCHECK(false);
    return false;
  }

  /// \todo memory copy
  const ::base::FilePath fp = ::base::FilePath(path);

  /// \todo use ::base::strlcpy
  const int result = ::base::strings::SafeSNPrintf(
    out_path, path_size, "%s",
    fp.BaseName().value().data()
    );
  if (result < 1 || result > kMaxPathSize) {
    out_path[0] = '\0';
    DCHECK(false);
    return false;
  }

  return true;
}

// Gets the path to a temporary directory that is unique to this process.
bool GetTemporaryDirectory(char* out_path, int path_size) {
  char binary_name[kMaxPathSize] = {0};
  if (!GetExecutableName(binary_name, kMaxPathSize)) {
    DCHECK(false);
    return false;
  }

  const int result
    = ::base::strings::SafeSNPrintf(
        out_path
        , path_size
        , "/tmp/%s-%d"
        , binary_name
        , static_cast<int>(getpid()));

  if (result < 1 || result > kMaxPathSize) {
    out_path[0] = '\0';
    DCHECK(false);
    return false;
  }

  return true;
}
}  // namespace

namespace application {

namespace paths {

bool AppGetPath(
  AppPathId path_id, char* out_path, int path_size)
{
  if (!out_path || !path_size) {
    DCHECK(false);
    return false;
  }

  char path[kMaxPathSize];
  path[0] = '\0';

  switch (path_id) {
    case paths::kAppPathContentDirectory: {
      if (!GetExecutableDirectory(path, kMaxPathSize)) {
        DCHECK(false);
        return false;
      }
      /// \todo use ::base::strlcpy
      const int result
        = ::base::strings::SafeSNPrintf(
            path
            , kMaxPathSize
            , "%s%s"
            , path
            , kAppContentDirRelative);
      if (result < 1 || result > kMaxPathSize) {
        DCHECK(false);
        return false;
      }
      break;
    }

    case paths::kAppPathCacheDirectory: {
      if (!GetCacheDirectory(path, kMaxPathSize)) {
        DCHECK(false);
        return false;
      }
      /// \todo use ::base::strlcpy
      const int result
        = ::base::strings::SafeSNPrintf(
            path
            , kMaxPathSize
            , "%s%s"
            , path
            , kAppCacheDirRelative);
      if (result < 1 || result > kMaxPathSize) {
        return false;
      }
      const bool dir_created
        = ::base::CreateDirectory(base::FilePath{path});
      if (!dir_created) {
        DLOG(ERROR)
          << "Unable to create directory "
          << path;
        DCHECK(false);
        return false;
      }
      break;
    }

    case paths::kAppPathDebugOutputDirectory: {
      if (!AppGetPath(paths::kAppPathTempDirectory, path, kMaxPathSize)) {
        DCHECK(false);
        return false;
      }

      /// \todo use ::base::strlcpy
      const int result
        = ::base::strings::SafeSNPrintf(
            path
            , kMaxPathSize
            , "%s%s"
            , path
            , "/log");
      if (result < 1 || result > kMaxPathSize) {
        DCHECK(false);
        return false;
      }

      const bool dir_created
        = ::base::CreateDirectory(base::FilePath{path});
      if(!dir_created){
        DLOG(ERROR)
          << "Unable to create directory "
          << path;
        DCHECK(false);
      }
      break;
    }

    case paths::kAppPathTempDirectory: {
      if (!GetTemporaryDirectory(path, kMaxPathSize)) {
        DCHECK(false);
        return false;
      }

      const bool dir_created
        = ::base::CreateDirectory(base::FilePath{path});
      if(!dir_created){
        DLOG(ERROR)
          << "Unable to create directory "
          << path;
        DCHECK(false);
      }
      break;
    }

    case paths::kAppPathTestOutputDirectory: {
      return AppGetPath(kAppPathDebugOutputDirectory
                        , out_path
                        , path_size);
    }

    // We return the library directory as the "executable" since:
    // a) Unlike the .so itself, it has a valid timestamp of the app install.
    // b) Its parent directory is still a directory within our app package.
    case paths::kAppPathExecutableFile: {
      return GetExecutablePath(out_path, path_size);
    }

    default:
      NOTIMPLEMENTED() << "AppGetPath not implemented for "
                       << path_id;
      return false;
  }

  /// \todo use ::base::strlcpy
  const int result = ::base::strings::SafeSNPrintf(
    out_path, path_size, "%s", path);

  /// \todo memory copy
  const ::base::StringPiece str = ::base::StringPiece{out_path};
  int length = str.length();
  if (length < 1 || length > path_size) {
    DCHECK(false);
    return false;
  }

  return true;
}

} // namespace paths

} // namespace application

