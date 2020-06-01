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
      , /*Size*/40
      , /*Alignment*/1
      , pimpl::SizePolicy::AtLeast
      , pimpl::AlignPolicy::AtLeast
    > m_impl;
};
