#pragma once

// DISABLE_DOCTEST: custom macro
#if !defined(DISABLE_DOCTEST)

#error "TODO: DOCTEST NOT SUPPORTED"

/// \note define DOCTEST_* macro before `doctest.h`
#include "basis/doctest_common.hpp"

// see doctest configuration at
// https://github.com/onqtam/doctest/blob/master/doc/markdown/configuration.md#doctest_config_disable
#if !defined(DOCTEST_CONFIG_DISABLE)

namespace doctest_util {

// create default doctest context (custom defaults)
doctest::Context initDoctest(
  int argc, const char* const* argv);

}  // namespace doctest_util

#endif // DOCTEST_CONFIG_DISABLE

#endif // DISABLE_DOCTEST
