#include "interval_timer.hpp" // IWYU pragma: associated

namespace basis {

void IntervalTimer::SetCurrent(const std::chrono::nanoseconds& current)
{
  DCHECK(current > std::chrono::nanoseconds::min()
    && current < std::chrono::nanoseconds::max());

  _current = current;
}

void IntervalTimer::SetInterval(const std::chrono::nanoseconds& interval)
{
  DCHECK(interval > std::chrono::nanoseconds{0}
    && interval < std::chrono::nanoseconds::max());

  _interval = interval;
}

IntervalTimer::IntervalTimer()
  : _interval(std::chrono::nanoseconds{0}), _current(std::chrono::nanoseconds{0})
{
}

IntervalTimer::IntervalTimer(const std::chrono::nanoseconds& interval)
  : _interval(interval), _current(std::chrono::nanoseconds{0})
{
}

} // namespace basis
