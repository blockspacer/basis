#include "basis/bind/delay_time_checker.hpp" // IWYU pragma: associated

namespace base {

DelayTimeChecker bindDelayTimeChecker(
  const ::base::Location& location
  , const ::base::TimeDelta& val)
{
  return DelayTimeChecker{
    location
    , val
  };
}

} // namespace base
