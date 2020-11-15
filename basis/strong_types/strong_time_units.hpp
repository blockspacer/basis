#pragma once

#include <base/macros.h>
#include <base/numerics/safe_conversions.h>
#include <base/numerics/checked_math.h>
#include <base/numerics/clamped_math.h>

#include <utility>
#include <type_traits>

namespace util {

DEFINE_STRONG_CHECKED_TYPE(CheckedSeconds, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedMilliseconds, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedMicroseconds, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedNanoseconds, uint64_t);

// USAGE
//
// util::CheckedMilliseconds valMs{2500};
// CHECK(util::millisecondsToSeconds(valMs).ValueOrDie() == 2);
//
inline CheckedSeconds millisecondsToSeconds(CheckedMilliseconds valMs)
{
  CheckedSeconds result(valMs.ValueOrDie());
  using type = CheckedSeconds::type;
  result /= base::saturated_cast<type>(1000UL);
  return result;
}

}  // namespace util
