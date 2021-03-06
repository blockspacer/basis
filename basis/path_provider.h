#pragma once

#include <base/macros.h>

#include <basic/macros.h>

namespace base {
class FilePath;
} // namespace base

namespace basis {

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

// returns path based on |ApplicationPathKeys|
MUST_USE_RETURN_VALUE
bool PathProvider(
  int key, ::base::FilePath* result);

extern
const char kAppPathDebugOutputDirectory[];

extern
const char kAppPathTestOutputDirectory[];

extern
const char kAppPathContentDirectory[];

// calls ::base::PathService::RegisterProvider
void AddPathProvider();

} // namespace basis
