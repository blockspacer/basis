#pragma once

#include <base/logging.h>

namespace application {

namespace paths {

// The following keys are used to retreive application-specific paths using
// base::PathService::Get().
//
// Example:
// --------
//   base::FilePath log_directory;
//   base::PathService::Get(paths::DIR_APP_DEBUG_OUT, &log_directory);
//

enum ApplicationPathKeys {
  // Unique key which should not collide with other path provider keys.
  PATH_APP_START = 1000,

  // Directory where all debug output (such as logs) should be stored.
  DIR_APP_DEBUG_OUT,

  // Directory where tests can write data such as expected results.
  DIR_APP_TEST_OUT,

  // Root directory for local web files (those fetched from file://).
  DIR_APP_WEB_ROOT,

  // End of path keys.
  PATH_APP_END,
};

} // namespace paths

} // namespace application
