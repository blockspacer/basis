#pragma once

#include "basis/application/platform_detection.hpp"

#if defined(OS_POSIX)
#include "posix/application_configuration_posix.hpp"
#endif // OS_POSIX

#if !defined(PLATFORM_FILE_MAX_PATH) || PLATFORM_FILE_MAX_PATH < 2
#error "Your platform must define PLATFORM_FILE_MAX_PATH > 1."
#endif

// Determines at compile-time an inherent aspect of this platform.
#define APP_IS(APP_FEATURE) \
  ((defined APP_IS_##APP_FEATURE) && APP_IS_##APP_FEATURE)

#include <base/compiler_specific.h>

#if !defined(ALLOW_THIS_IN_INITIALIZER_LIST)

#if defined(COMPILER_MSVC)
// Allows |this| to be passed as an argument in constructor initializer lists.
// This uses push/pop instead of the seemingly simpler suppress feature to avoid
// having the warning be disabled for more than just |code|.
//
// Example usage:
// Foo::Foo() : x(NULL), ALLOW_THIS_IN_INITIALIZER_LIST(y(this)), z(3) {}
//
// Compiler warning C4355: 'this': used in base member initializer list:
// http://msdn.microsoft.com/en-us/library/3c594ae3(VS.80).aspx
#define ALLOW_THIS_IN_INITIALIZER_LIST(code) \
  MSVC_PUSH_DISABLE_WARNING(4355)            \
  code \
  MSVC_POP_WARNING()

#else  // Not MSVC

#define ALLOW_THIS_IN_INITIALIZER_LIST(code) \
  code

#endif  // COMPILER_MSVC

#endif  // ALLOW_THIS_IN_INITIALIZER_LIST
