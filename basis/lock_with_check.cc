#include "basis/lock_with_check.hpp" // IWYU pragma: associated

namespace basis {

/// \note It is not real lock, only annotated as lock.
/// It just calls callback on scope entry AND exit.
basis::FakeLockWithCheck<bool()>
  fakeLockDocumentNotThreadChecked {
    basis::VerifyNothing::Repeatedly()
  };

} // namespace basis
