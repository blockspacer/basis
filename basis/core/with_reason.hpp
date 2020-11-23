#pragma once

#include <string>

namespace basis {

// In functions with priority-ordered return statements, this helps with
// identifying the statement that took effect.
//
// Instead of writing:
// int f() {
//   ...
//   return value1;   // explanation1
//   ...
//   return value2;   // explanation2
// }
//
// Write:
// WithReason<int> f() {
//   ...
//   return {value1; "explanation1..."};
//   ...
//   return {value2; "explanation2..."};
// }
//
// The caller should unpack the struct, and can print the reason string
// when desired.
template <typename T>
struct WithReason {
  T value;
  const char* reason;  // A simple string literal shall suffice.
};

}  // namespace basis
