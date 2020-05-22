#include "Foo.h"

#include <basis/core/pimpl.hpp>

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class Foo::FooImpl
{
 public:
  FooImpl() = default;

  ~FooImpl() = default;

  int foo();
};

int Foo::FooImpl::foo() {
  return 1234;
}

Foo::Foo()
{}

Foo::~Foo()
{}

int Foo::foo() {
  return m_impl->foo();
}
