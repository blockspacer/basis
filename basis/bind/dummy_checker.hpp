#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>

#include <basis/scoped_checks.hpp>

namespace base {

template <typename... Args>
class DummyChecker
{
 public:
  GUARD_METHOD_ON_UNKNOWN_THREAD(DummyChecker)
  DummyChecker(Args&&... args)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(DummyChecker);

    /// \note fold expression requires C++17
    ((void)(ignore_result(args)), ...);
  }

  // check call count on callback destruction
  ~DummyChecker()
  {}

  DummyChecker(DummyChecker<Args...>&& other)
  {}

  DummyChecker& operator=(
    DummyChecker<Args...>&& other)
  {
    return *this;
  }

  void runCheckBeforeInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckBeforeInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckBeforeInvoker);
  }

  void runCheckAfterInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckAfterInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckAfterInvoker);
  }

 private:
  // Object construction can be on any thread
  CREATE_METHOD_GUARD(DummyChecker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckBeforeInvoker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckAfterInvoker);

  DISALLOW_COPY_AND_ASSIGN(DummyChecker);
};

} // namespace base
