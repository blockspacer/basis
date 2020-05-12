#pragma once

#include <chrono>
#include <base/logging.h>

namespace basis {

class ScopedLogRunTime {
public:
  ScopedLogRunTime(
    std::chrono::steady_clock::time_point chrono_then
      = std::chrono::steady_clock::now());

  ~ScopedLogRunTime();

private:
  std::chrono::steady_clock::time_point chrono_then_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLogRunTime);
};

}  // namespace basis
