#include "testsCommon.h"

#include "Foo.h"

#if !defined(USE_CATCH_TEST)
#warning "use USE_CATCH_TEST"
// default
#define USE_CATCH_TEST 1
#endif // !defined(USE_CATCH_TEST)

#include "basis/core/pimpl.hpp"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

SCENARIO("pimpl", "[basic]") {

  GIVEN("FastPimplExample") {
    Foo foo;

    REQUIRE(foo.foo() == 1234);
  }
}
