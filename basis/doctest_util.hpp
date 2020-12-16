#pragma once

// DISABLE_DOCTEST: custom macro
#if !defined(DISABLE_DOCTEST)

/// \note define DOCTEST_* macro before `doctest.h`
#include "basis/doctest_common.hpp"

// see doctest configuration at
// https://github.com/onqtam/doctest/blob/master/doc/markdown/configuration.md#doctest_config_disable
#if !defined(DOCTEST_CONFIG_DISABLE)

namespace basis {

void initDoctestOptions(doctest::Context& ctx);

}  // namespace basis

#endif // DOCTEST_CONFIG_DISABLE

#endif // DISABLE_DOCTEST
