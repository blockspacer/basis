#pragma once

#include "basis/application/application_configuration.hpp"
#include "basis/application/export.hpp"
#include "basis/application/paths/path_id.hpp"

namespace application {

namespace paths {

/// \note This implementation must be thread-safe.
// Retrieves the platform-defined system path specified by |path_id| and
// places it as a zero-terminated string into the user-allocated |out_path|
// unless it is longer than |path_length| - 1.
//
// This function returns |true| if the path is retrieved successfully. It
// returns |false| under any of the following conditions and, in any such
// case, |out_path| is not changed:
// - |path_id| is invalid for this platform
// - |path_length| is too short for the given result
// - |out_path| is NULL
//
// |path_id|: The system path to be retrieved.
// |out_path|: The platform-defined system path specified by |path_id|.
// |path_length|: The length of the system path.
APP_EXPORT bool AppGetPath(AppPathId path_id,
                               char* out_path,
                               int path_length);

} // namespace paths

} // namespace application
