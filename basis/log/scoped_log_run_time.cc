#include "basis/log/scoped_log_run_time.hpp" // IWYU pragma: associated

#include <base/timer/elapsed_timer.h>

namespace basis {

ScopedLogRunTime::ScopedLogRunTime(logging::LogSeverity severity, const std::string& prefix)
  : timer_(std::make_unique<::base::ElapsedTimer>())
  , severity_(severity)
  , prefix_(prefix)
{}

basis::ScopedLogRunTime::~ScopedLogRunTime()
{
  ::base::TimeDelta elapsed_delta = timer_->Elapsed();
  if(::logging::ShouldCreateLogMessage(severity_))
  {
    ::logging::LogMessage(__FILE__, __LINE__, severity_).stream()
      << prefix_
      << "Done in : "
      << elapsed_delta.InMilliseconds() << " milliseconds"
      <<" (" << elapsed_delta.InNanoseconds() << " nanoseconds)";
  }
}

}  // namespace basis
