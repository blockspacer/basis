#pragma once

#include "basis/application/application_configuration.hpp"

// APP_EXPORT: Specification for a symbol that should be exported when building
// the DLL and imported when building code that uses the DLL.

// APP_EXPORT_PRIVATE: Specification for a symbol that should be exported or
// imported for testing purposes only.

// APP_IMPORT: Specification for a symbol that is expected to be defined
// externally to this module.

#if defined(APP_COMPONENT_BUILD) || APP_IS(APP_EVERGREEN)
// APP_COMPONENT_BUILD is defined when generating shared libraries for each project,
// rather than static libraries. This means we need to be careful about
// EXPORT/IMPORT.

#if defined(APP_IMPLEMENTATION)
// APP_IMPLEMENTATION is defined when building the
// sources, and shouldn't be defined when building sources for clients
#define APP_EXPORT APP_EXPORT_PLATFORM
#define APP_EXPORT_PRIVATE APP_EXPORT_PLATFORM
#define APP_IMPORT APP_IMPORT_PLATFORM
#else  // defined(APP_IMPLEMENTATION)
#define APP_EXPORT APP_IMPORT_PLATFORM
#define APP_EXPORT_PRIVATE APP_IMPORT_PLATFORM
#define APP_IMPORT APP_EXPORT_PLATFORM
#endif
#else  // defined(APP_COMPONENT_BUILD) || APP_IS(APP_EVERGREEN)
#define APP_EXPORT
#define APP_EXPORT_PRIVATE
#define APP_IMPORT
#endif  // defined(APP_COMPONENT_BUILD)
