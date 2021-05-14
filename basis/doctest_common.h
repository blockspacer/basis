#pragma once

// file configures doctest macros
// see https://github.com/onqtam/doctest/blob/master/doc/markdown/configuration.md
/// \note that header file exports doctest.h
/// with custom defines.
/// We assume that if DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
/// was defined, than `doctest.h` will contain only
/// `dummy` defines like
/// `#define DOCTEST_FAIL(x) ((void)0)`
/// and including `doctest.h` will not affect build time

// DISABLE_DOCTEST: custom macro
#if !defined(DISABLE_DOCTEST)

#define DOCTEST_CONFIG_USE_STD_HEADERS

#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS

#define DOCTEST_CONFIG_NO_TRY_CATCH_IN_ASSERTS

#define DOCTEST_CONFIG_NO_EXCEPTIONS

// `#define INFO` from DOCTEST
// conflicts with macros from other libs
#define DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES

/// \note define DOCTEST_* macro before `doctest.h`
#include <doctest/doctest.h>

#endif // DISABLE_DOCTEST
