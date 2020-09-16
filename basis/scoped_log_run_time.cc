#include "basis/scoped_log_run_time.hpp" // IWYU pragma: associated

#include <base/timer/elapsed_timer.h>

namespace basis {

ScopedLogRunTime::ScopedLogRunTime()
  : timer_(std::make_unique<base::ElapsedTimer>())
{}

basis::ScopedLogRunTime::~ScopedLogRunTime()
{
  base::TimeDelta elapsed_delta = timer_->Elapsed();

  LOG(INFO)
    << "Done in : "
    << elapsed_delta.InMilliseconds() << " milliseconds"
    <<" (" << elapsed_delta.InNanoseconds() << " nanoseconds)";
}

}  // namespace basis
