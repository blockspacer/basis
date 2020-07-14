#pragma once

#include <base/macros.h>

#include <memory>

namespace base {
class ElapsedTimer;
} // namespace base

namespace basis {

class ScopedLogRunTime {
public:
  ScopedLogRunTime();

  ~ScopedLogRunTime();

private:
  std::unique_ptr<base::ElapsedTimer> timer_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLogRunTime);
};

}  // namespace basis
