#pragma once

#include <base/macros.h>
#include <base/logging.h>

#include <memory>
#include <string>
#include <cstdint>
#include <iosfwd>

// USAGE
//
// bool has_err = ...;
// LOG_TIMING_IF(has_err, &LOG_STREAM(ERROR), "Some calculations ")
// {
//   //  ... Some calculations ...
// }
//
#define LOG_TIMING_IF(condition, ...) \
  if (basis::ScopedLogRunTime internal_scopedLogRunTime(__VA_ARGS__); condition)

// USAGE
//
// LOG_TIMING(&LOG_STREAM(INFO), "Some calculations ")
// {
//   //  ... Some calculations ...
// }
//
#define LOG_TIMING(...) \
  LOG_TIMING_IF(true, __VA_ARGS__)

namespace base {
class ElapsedTimer;
} // namespace base

namespace basis {

// USAGE
//
// {
//   ::basis::ScopedLogRunTime scopedLogRunTime{};
//   //  ... Some calculations ...
// }
//
// {
//   ::basis::ScopedLogRunTime scopedLogRunTime{&LOG_STREAM(INFO), "Some calculations "};
//   //  ... Some calculations ...
// }
//
class ScopedLogRunTime {
public:
  ScopedLogRunTime(std::ostream* out = &LOG_STREAM(INFO), const std::string& prefix = "");

  ~ScopedLogRunTime();

private:
  std::unique_ptr<::base::ElapsedTimer> timer_;

  std::string prefix_;

  std::ostream* stream_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLogRunTime);
};

}  // namespace basis
