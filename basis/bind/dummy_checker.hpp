#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>

#include <basis/checks_and_guard_annotations.hpp>

namespace base {

template <typename... Args>
class DummyChecker
{
 public:
  DummyChecker(Args&&... args)
  {
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
  {}

  void runCheckAfterInvoker()
  {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DummyChecker);
};

} // namespace base
