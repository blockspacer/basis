#include "tests_common.h"

#include "basis/strong_types/strong_alias.hpp"

#include <cstddef>
#include <cstdint>

namespace base {

using Orange = ::basis::StrongAlias<class OrangeTag, int>;

STRONGLY_TYPED(int, Apple);

void foo(Orange)
{}

// Compiles into separate overload.
void foo(Apple)
{}

TEST(StrongIntTest, Simple) {
  Apple apple(2);

  // Orange orange = apple; // Does not compile.

  int num = 3;

  Orange orange = num;

  orange = 3;
  EXPECT_EQ(orange, 3);

  orange = 3UL;
  EXPECT_EQ(orange, 3UL);

  Orange other_orange = orange;

  // Orange x = orange + apple; // Does not compile.

  Orange y = Orange(orange.value() + apple.value());

  // if (orange > apple);  // Does not compile.

  EXPECT_GT(orange, other_orange);
}

}  // namespace base
