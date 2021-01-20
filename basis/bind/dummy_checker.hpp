#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>

#include <basis/checks_and_guard_annotations.hpp>

namespace base {

template <typename... Args>
class DummyChecker
{
 public:
  GUARD_NOT_THREAD_BOUND_METHOD(DummyChecker)
  DummyChecker(Args&&... args)
  {
    DCHECK_NOT_THREAD_BOUND_METHOD(DummyChecker);

    ((void)(UNREFERENCED_PARAMETER(args)), ...);
  }

  // check call count on callback destruction
  ~DummyChecker()
  {}

  DummyChecker(DummyChecker<Args...>&& other)
  {
    UNREFERENCED_PARAMETER(other);
  }

  DummyChecker& operator=(
    DummyChecker<Args...>&& other)
  {
    UNREFERENCED_PARAMETER(other);
    return *this;
  }

  void runCheckBeforeInvoker()
  GUARD_NOT_THREAD_BOUND_METHOD(runCheckBeforeInvoker)
  {
    DCHECK_NOT_THREAD_BOUND_METHOD(runCheckBeforeInvoker);
  }

  void runCheckAfterInvoker()
  GUARD_NOT_THREAD_BOUND_METHOD(runCheckAfterInvoker)
  {
    DCHECK_NOT_THREAD_BOUND_METHOD(runCheckAfterInvoker);
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
