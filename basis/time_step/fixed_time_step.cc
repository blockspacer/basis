#include "basis/time_step/fixed_time_step.h" // IWYU pragma: associated

namespace basis {

FixedTimeStep::FixedTimeStep(const std::chrono::nanoseconds& tickrate) noexcept
  : fps_{tickrate}
  , fixed_delta_time_{std::chrono::duration<float, std::ratio<1>>(tickrate).count()}
  , fixed_tickrate_{tickrate}
{
  DCHECK(tickrate != std::chrono::nanoseconds::min());
  DCHECK(tickrate != std::chrono::nanoseconds::max());
}

} // namespace basis
