#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>

#include <basis/scoped_checks.hpp>

namespace base {

// Check lifetime of pointer, use memory tool like ASAN
//
// USAGE
//
// {
//   // ERROR: AddressSanitizer: stack-use-after-scope
//   base::MessageLoop::current().task_runner()->PostTask(
//     FROM_HERE
//     , base::bindCheckedOnce(
//         DEBUG_BIND_CHECKS(
//           REF_CHECKER(tmpClass)
//         )
//         , &TmpClass::TestMe
//         , base::Unretained(&tmpClass)
//         , base::Passed(FROM_HERE))
//   );
//
//   DVLOG(9)
//     << "TmpClass freed before `runLoop.Run()`"
//     << " i.e. before `PostTask` execution"
//     << " Build with `-fsanitize=address,undefined`"
//     << " to detect `AddressSanitizer: stack-use-after-scope`";
// }
#define REF_CHECKER(REF_NAME) \
  base::bindRefChecker(FROM_HERE, REFERENCED(REF_NAME))

#if DCHECK_IS_ON()
#define DEBUG_REF_CHECKER(PTR_NAME) \
  REF_CHECKER(PTR_NAME)
#else
#define DEBUG_REF_CHECKER(PTR_NAME) \
  DUMMY_CHECKER(PTR_NAME)
#endif

// Check lifetime of reference, use memory tool like ASAN
//
// USAGE
//
// {
//   // ERROR: AddressSanitizer: stack-use-after-scope
//   base::MessageLoop::current().task_runner()->PostTask(
//     FROM_HERE
//     , base::bindCheckedOnce(
//         DEBUG_BIND_CHECKS(
//           PTR_CHECKER(&tmpClass)
//         )
//         , &TmpClass::TestMe
//         , base::Unretained(&tmpClass)
//         , base::Passed(FROM_HERE))
//   );
//
//   DVLOG(9)
//     << "TmpClass freed before `runLoop.Run()`"
//     << " i.e. before `PostTask` execution"
//     << " Build with `-fsanitize=address,undefined`"
//     << " to detect `AddressSanitizer: stack-use-after-scope`";
// }
#define PTR_CHECKER(PTR_NAME) \
  base::bindPtrChecker(FROM_HERE, PTR_NAME)

#if DCHECK_IS_ON()
#define DEBUG_PTR_CHECKER(PTR_NAME) \
  PTR_CHECKER(PTR_NAME)
#else
#define DEBUG_PTR_CHECKER(PTR_NAME) \
  DUMMY_CHECKER(PTR_NAME)
#endif

template <typename PtrType>
class PtrChecker
{
 public:
  template <typename U>
  GUARD_METHOD_ON_UNKNOWN_THREAD(PtrChecker)
  explicit
  PtrChecker(
    const base::Location& location
    , U* ptr)
    : ptr_(ptr)
    , location_(location)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(PtrChecker);

    /// \note disallows nullptr
    CHECK(ptr_)
      << location_.ToString();
    checkForLifetimeIssues();
  }

  // called on callback destruction
  ~PtrChecker()
  {
  }

  PtrChecker(PtrChecker<PtrType>&& other)
    : ptr_{base::rvalue_cast(other.ptr_)}
    , location_{base::rvalue_cast(other.location_)}
  {}

  PtrChecker& operator=(
    PtrChecker<PtrType>&& other)
  {
    ptr_ = base::rvalue_cast(other.ptr_);
    location_ = base::rvalue_cast(other.location_);
    return *this;
  }

  void runCheckBeforeInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckBeforeInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckBeforeInvoker);

    /// \note disallows nullptr
    CHECK(ptr_)
      << location_.ToString();
    checkForLifetimeIssues();
  }

  void runCheckAfterInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckAfterInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckAfterInvoker);
  }

 private:
  // check that object is alive, use memory tool like ASAN
  inline void checkForLifetimeIssues() const
  {
    // Works with `-fsanitize=address,undefined
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
    if (ptr_)
      reinterpret_cast<const volatile uint8_t*>(ptr_)[0];
#endif
  }

 private:
  PtrType* ptr_ = nullptr;

  base::Location location_;

  // Object construction can be on any thread
  CREATE_METHOD_GUARD(PtrChecker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckBeforeInvoker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckAfterInvoker);

  DISALLOW_COPY_AND_ASSIGN(PtrChecker);
};

template <typename PtrType>
PtrChecker<PtrType> bindPtrChecker(
  const base::Location& location
  , PtrType* ptr)
{
  return PtrChecker<PtrType>{location, ptr};
}

} // namespace base
