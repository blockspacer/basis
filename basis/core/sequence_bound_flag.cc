#include "basis/core/sequence_bound_flag.hpp" // IWYU pragma: associated

#include <base/logging.h>

namespace basis {

SequenceBoundFlag::SequenceBoundFlag(bool value)
  : value_(value)
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

} // namespace basis
