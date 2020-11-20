#pragma once

#include <base/logging.h>

namespace basis {

/// \note `polymorphic_upcast` usually safe (unlike `polymorphic_downcast`),
/// so prefer it for documentation purposes (not because of extra checks)
//
// Upcasting means casting from a derived class to a base class.
//
// EXAMPLE:
//   ...
//   class Fruit { public: virtual ~Fruit(){}; ... };
//   class Banana : public Fruit { ... };
//   ...
//   void f( Banana * banana ) {
//   // ... logic which leads us to believe it is a fruit
//     Fruit * fruit = polymorphic_upcast<Fruit*>(banana);
//     ...
template <typename Base, typename Derived>
Base polymorphic_upcast(Derived derived) {
#if DCHECK_IS_ON()
  DCHECK(dynamic_cast<Base>(derived) == derived);
#endif // DCHECK_IS_ON
  return static_cast<Base>(derived);
}

}  // namespace basis
