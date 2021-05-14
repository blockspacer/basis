// DISABLE_DOCTEST: custom macro
#if !defined(DISABLE_DOCTEST)

/// \note place before "basis/doctest_util.h"
/// \note define DOCTEST_* macro before `doctest.h`
#include "basis/doctest_common.h"

#include "basis/doctest_util.h" // IWYU pragma: associated

#include <utility>

// see doctest configuration at
// https://github.com/onqtam/doctest/blob/master/doc/markdown/configuration.md#doctest_config_disable
#if !defined(DOCTEST_CONFIG_DISABLE)

namespace basis {

void initDoctestOptions(doctest::Context& ctx)
{
  // default - stop after 5 failed asserts
  ctx.setOption("abort-after", 5);

  // override - do not break in the debugger
  ctx.setOption("no-breaks", true);

  // include successful assertions in output
  ctx.setOption("success", true);

  // Print test names to output as they are run
  ctx.setOption("print_test", true);

  // prints the time duration of each test
  ctx.setOption("duration", true);
}

}  // namespace basis

#endif // DOCTEST_CONFIG_DISABLE

#endif // DISABLE_DOCTEST
