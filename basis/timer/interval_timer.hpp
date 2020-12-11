#pragma once

#include <chrono>
#include <base/logging.h> // for DCHECK

namespace basis {

// Usage example:
//  in main loop you want to update sub-systems with some interval,
//  so you can pass elapsed frame time_dt to IntervalTimer::Update
//  and use IntervalTimer::Passed to check if sub-systems update interval reached,
//  then do not forget to call IntervalTimer::Reset()
struct IntervalTimer
{
 public:
  IntervalTimer();

  IntervalTimer(const std::chrono::nanoseconds& interval);

  inline /* `inline` to eleminate function call overhead */
  void Update(const std::chrono::nanoseconds& diff) noexcept
  {
    DCHECK(diff > std::chrono::nanoseconds::min()
      && diff < std::chrono::nanoseconds::max());

    _current += diff;

    if (_current < std::chrono::nanoseconds{0})
      _current = std::chrono::nanoseconds{0};
  }

  inline /* `inline` to eleminate function call overhead */
  bool Passed() noexcept
  {
    return _current >= _interval;
  }

  /// \note keeps remainder by `%= _interval`
  inline /* `inline` to eleminate function call overhead */
  void Reset() noexcept
  {
    DCHECK(_interval != std::chrono::nanoseconds{0});

    if (_current >= _interval)
      _current %= _interval;
  }

  inline /* `inline` to eleminate function call overhead */
  const std::chrono::nanoseconds GetCurrent() const noexcept
  {
    return _current;
  }

  inline /* `inline` to eleminate function call overhead */
  const std::chrono::nanoseconds GetInterval() const noexcept
  {
    return _interval;
  }

  void SetCurrent(const std::chrono::nanoseconds& current);

  void SetInterval(const std::chrono::nanoseconds& interval);

 private:
  std::chrono::nanoseconds _interval;
  std::chrono::nanoseconds _current;
};

} // namespace basis
