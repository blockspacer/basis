#pragma once

#include <basis/core/pimpl.hpp>

class Foo {
public:
  Foo();

  ~Foo();

  int foo();

private:
  class FooImpl;

  ::basis::FastPimpl<
      FooImpl
      , /*Size*/40
      , /*Alignment*/1
      , ::basis::pimpl::SizePolicy::AtLeast
      , ::basis::pimpl::AlignPolicy::AtLeast
    > m_impl;
};
