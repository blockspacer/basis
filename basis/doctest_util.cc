/// \note that file defines DOCTEST_CONFIG_IMPLEMENT
/// so you do not need to define it again in executable

// DISABLE_DOCTEST: custom macro
#if !defined(DISABLE_DOCTEST)

// DOCTEST_CONFIG_IMPLEMENT:
// If the client wants to supply the main() function
// (either to set an option with some value from the code
// or to integrate the framework into his existing project codebase)
// this identifier should be used.
// This should be defined only in the source file
// where the library is implemented.
#define DOCTEST_CONFIG_IMPLEMENT

/// \note place before "basis/doctest_util.hpp"
/// \note define DOCTEST_* macro before `doctest.h`
#include "basis/doctest_common.hpp"

/// \note already includes `doctest.h`,
/// but we want to include `doctest.h`
/// with DOCTEST_CONFIG_IMPLEMENT
/// so place it below custom `doctest.h`
#include "basis/doctest_util.hpp" // IWYU pragma: associated

#include <utility> // std::move

// see doctest configuration at
// https://github.com/onqtam/doctest/blob/master/doc/markdown/configuration.md#doctest_config_disable
#if !defined(DOCTEST_CONFIG_DISABLE)

namespace doctest_util {

doctest::Context initDoctest(
  int argc, const char* const* argv)
{
  doctest::Context ctx;

  // default - stop after 5 failed asserts
  ctx.setOption("abort-after", 5);

  // apply command line - argc / argv
  ctx.applyCommandLine(argc, argv);

  // override - do not break in the debugger
  ctx.setOption("no-breaks", true);

  return std::move(ctx);
}

}  // namespace doctest_util

#endif // DOCTEST_CONFIG_DISABLE

#endif // DISABLE_DOCTEST
