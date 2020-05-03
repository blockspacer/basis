#pragma once

/// \note Similar to [[nodiscard]] attribute,
/// being implemented by Clang and GCC as __attribute__((warn_unused_result))
///
// Annotate a function indicating the caller must examine the return value.
// Use like:
//   WARN_DISCARDED_RESULT int foo();
// To explicitly ignore a result, see |ignore_result()| in base/macros.h.
#undef WARN_DISCARDED_RESULT
#if defined(COMPILER_GCC) || defined(__clang__)
#define WARN_DISCARDED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_DISCARDED_RESULT
#endif
