#pragma once

#include <base/macros.h>
#include <base/numerics/safe_conversions.h>
#include <base/numerics/checked_math.h>
#include <base/numerics/clamped_math.h>

#include <utility>
#include <type_traits>

namespace util {

DEFINE_STRONG_CHECKED_TYPE(CheckedBytes, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedKilobytes, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedMegabytes, uint64_t);
DEFINE_STRONG_CHECKED_TYPE(CheckedGigabytes, uint64_t);

// USAGE
//
// util::CheckedBytes valBytes{2500000};
// CHECK(util::bytesToMegabytes(valBytes).ValueOrDie() == 2);
//
inline CheckedMegabytes bytesToMegabytes(CheckedBytes bytes)
{
  CheckedMegabytes result(bytes.ValueOrDie());
  using type = CheckedMegabytes::type;
  result >>= ::base::saturated_cast<type>(20UL);
  return result;
}

// USAGE
//
// util::CheckedKilobytes valKb{5};
// CHECK(util::kilobytesToBytes(valKb).ValueOrDie() == 5120);
//
inline CheckedBytes kilobytesToBytes(CheckedKilobytes kilobytes)
{
  CheckedBytes result(kilobytes.ValueOrDie());
  using type = CheckedBytes::type;
  result <<= ::base::saturated_cast<type>(10UL);
  return result;
}

// USAGE
//
// util::CheckedMegabytes valMb{1};
// CHECK(util::megabytesToBytes(valMb).ValueOrDie() == 1048576);
//
inline CheckedBytes megabytesToBytes(CheckedMegabytes megabytes)
{
  CheckedBytes result(megabytes.ValueOrDie());
  using type = CheckedBytes::type;
  result <<= ::base::saturated_cast<type>(20UL);
  return result;
}

// USAGE
//
// util::CheckedGigabytes valGb{1};
// CHECK(util::gigabytesToBytes(valGb).ValueOrDie() == 1073741824);
//
inline CheckedBytes gigabytesToBytes(CheckedGigabytes gigabytes)
{
  CheckedBytes result(gigabytes.ValueOrDie());
  using type = CheckedBytes::type;
  result <<= ::base::saturated_cast<type>(30UL);
  return result;
}

}  // namespace util
