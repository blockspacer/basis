#include "basis/scoped_log_run_time.hpp" // IWYU pragma: associated

#include <base/logging.h>

namespace basis {

ScopedLogRunTime::ScopedLogRunTime(
  std::chrono::steady_clock::time_point chrono_then)
  : chrono_then_(chrono_then)
{}

basis::ScopedLogRunTime::~ScopedLogRunTime()
{
  long int diff_ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - chrono_then_)
      .count();
  long int diff_ns
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - chrono_then_)
      .count();
  DLOG(INFO)
      << "Done in : "
      << diff_ms << " milliseconds"
      <<" (" << diff_ns << " nanoseconds)";
}

}  // namespace basis
