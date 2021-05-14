#pragma once

#include <base/files/file_path.h>

namespace application {

namespace paths {

// Usage:
// ::base::PathService::RegisterProvider(&application::PathProvider,
//                                       application::paths::PATH_APP_START,
//                                       application::paths::PATH_APP_END);
bool PathProvider(int key, ::base::FilePath* result);

} // namespace paths

} // namespace application
