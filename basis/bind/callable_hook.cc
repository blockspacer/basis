#include "basis/bind/callable_hook.hpp" // IWYU pragma: associated

namespace basis {

GlobalCallableHooksRegistry::CallableSlot::~CallableSlot()
{}

// static
GlobalCallableHooksRegistry* GlobalCallableHooksRegistry::GetInstance()
{
  /// \note assumed to be thread-safe due to `base::NoDestructor`
  /// and constructed on first access
  static base::NoDestructor<GlobalCallableHooksRegistry> instance;
  return instance.get();
}

} // namespace basis
