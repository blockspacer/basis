#pragma once

#include <basis/core/pimpl.hpp>

class Foo {
public:
  Foo();

  ~Foo();

  int foo();

private:
  class FooImpl;

  pimpl::FastPimpl<
      FooImpl
      , /*Size*/1
      , /*Alignment*/1
    > m_impl;
};
