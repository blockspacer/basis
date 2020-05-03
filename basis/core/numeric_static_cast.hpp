#pragma once

#include <cassert>
#include <limits>
#include <base/logging.h>

/// \note in most cases use `saturated_cast` or `strict_cast`
/// see https://github.com/blockspacer/chromium_base_conan/blob/8e45a5dc6abfc06505fd660c08ad43c592daf5aa/base/numerics/safe_conversions.h#L201

// checks during runtime
// if there is a value overflow/underflow when using static_cast
//
// EXAMPLE (g++):
// int main() {
//   std::int64_t ll
//     = std::numeric_limits<std::int64_t>::max();
//   ++ll;
//   std::cout << ll << "\n"; // -9223372036854775808
//   std::int32_t t
//     = static_cast<std::int32_t>(ll);
//   std::cout << t << "\n"; // 0
//   std::int32_t m
//     = checked_static_cast<std::int32_t>(ll);
//   std::cout << m << "\n"; // "Cast truncates value"' failed.
// }
template <typename T, typename U>
T numeric_static_cast(U value) {
  DCHECK(static_cast<U>(static_cast<T>(value)) == value
           && "Cast truncates value");
  DCHECK(value >= std::numeric_limits<T>::min());
  DCHECK(value <= std::numeric_limits<T>::max());
  return static_cast<T>(value);
}
